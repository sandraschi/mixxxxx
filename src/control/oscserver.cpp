#include "control/oscserver.h"

#include <cstring>

#include <QtEndian>

#include <QRegularExpression>
#include <QUdpSocket>

#include "control/controlobject.h"
#include "control/controlproxy.h"
#include "mixer/playermanager.h"
#include "moc_oscserver.cpp"
#include "util/logging.h"

namespace mixxx {
namespace {

constexpr char kMasterGroup[] = "[Master]";
constexpr char kMicrophoneGroup[] = "[Microphone]";
constexpr char kExportGroup[] = "[Export]";

int readPaddedString(const QByteArray& data, int offset, QString* pOut) {
    if (offset >= data.size()) {
        return -1;
    }
    const int start = offset;
    while (offset < data.size() && data.at(offset) != '\0') {
        ++offset;
    }
    *pOut = QString::fromUtf8(data.constData() + start, offset - start);
    ++offset;
    while (offset % 4 != 0) {
        ++offset;
    }
    return offset;
}

void appendPaddedString(QByteArray* data, const QString& text) {
    data->append(text.toUtf8());
    data->append('\0');
    while (data->size() % 4 != 0) {
        data->append('\0');
    }
}

float readFloat32BE(const QByteArray& data, int offset) {
    if (offset + 4 > data.size()) {
        return 0.0f;
    }
    quint32 be = qFromBigEndian<quint32>(
            *reinterpret_cast<const quint32*>(data.constData() + offset));
    float value = 0.0f;
    static_assert(sizeof(float) == sizeof(quint32));
    std::memcpy(&value, &be, sizeof(float));
    return value;
}

void appendFloat32BE(QByteArray* data, float value) {
    quint32 be = 0;
    std::memcpy(&be, &value, sizeof(float));
    be = qToBigEndian(be);
    data->append(reinterpret_cast<const char*>(&be), sizeof(be));
}

QString channelGroupForDeck(int deck) {
    return QStringLiteral("[Channel%1]").arg(deck);
}

} // namespace

OscServer::OscServer(PlayerManager* pPlayerManager,
        UserSettingsPointer pConfig,
        QObject* pParent)
        : QObject(pParent),
          m_pPlayerManager(pPlayerManager),
          m_pConfig(std::move(pConfig)),
          m_pReceiveSocket(new QUdpSocket(this)),
          m_pSendSocket(new QUdpSocket(this)),
          m_sendHost(QHostAddress::LocalHost),
          m_sendPort(kDefaultPortOut),
          m_running(false) {
}

OscServer::~OscServer() {
    stop();
}

void OscServer::start() {
    if (m_running) {
        return;
    }

    const bool enabled = m_pConfig->getValue<bool>(ConfigKey("[Osc]", "enabled"), true);
    if (!enabled) {
        qInfo() << "OSC server disabled via [Osc],enabled";
        return;
    }

    const int portIn = m_pConfig->getValue<int>(
            ConfigKey("[Osc]", "port_in"), kDefaultPortIn);
    m_sendPort = static_cast<quint16>(m_pConfig->getValue<int>(
            ConfigKey("[Osc]", "port_out"), kDefaultPortOut));
    const QString hostOut = m_pConfig->getValueString(ConfigKey("[Osc]", "host_out"));
    if (!hostOut.isEmpty()) {
        m_sendHost = QHostAddress(hostOut);
    }

    if (!m_pReceiveSocket->bind(QHostAddress::Any, static_cast<quint16>(portIn))) {
        qWarning() << "OSC server failed to bind port" << portIn << m_pReceiveSocket->errorString();
        return;
    }

    connect(m_pReceiveSocket,
            &QUdpSocket::readyRead,
            this,
            &OscServer::slotReadPendingDatagrams);

    setupSubscriptions();
    m_running = true;
    qInfo().noquote() << QStringLiteral("OSC server listening on :%1, feedback to %2:%3")
                                 .arg(portIn)
                                 .arg(m_sendHost.toString())
                                 .arg(m_sendPort);
}

void OscServer::stop() {
    if (!m_running) {
        return;
    }
    m_subscriptions.clear();
    m_pReceiveSocket->close();
    m_running = false;
}

bool OscServer::decodeMessage(
        const QByteArray& datagram, QString* pAddress, QVector<QVariant>* pArgs) {
    if (datagram.size() < 4 || pAddress == nullptr || pArgs == nullptr) {
        return false;
    }

    int offset = 0;
    offset = readPaddedString(datagram, offset, pAddress);
    if (offset < 0) {
        return false;
    }
    if (offset >= datagram.size()) {
        return true;
    }

    QString typeTags;
    offset = readPaddedString(datagram, offset, &typeTags);
    if (offset < 0 || !typeTags.startsWith(',')) {
        return false;
    }

    pArgs->clear();
    for (int i = 1; i < typeTags.size(); ++i) {
        const char tag = typeTags.at(i).toLatin1();
        if (tag == 'f') {
            if (offset + 4 > datagram.size()) {
                return false;
            }
            pArgs->append(readFloat32BE(datagram, offset));
            offset += 4;
        } else if (tag == 'i') {
            if (offset + 4 > datagram.size()) {
                return false;
            }
            const qint32 value = static_cast<qint32>(qFromBigEndian<quint32>(
                    *reinterpret_cast<const quint32*>(datagram.constData() + offset)));
            pArgs->append(value);
            offset += 4;
        } else if (tag == 's') {
            QString str;
            offset = readPaddedString(datagram, offset, &str);
            if (offset < 0) {
                return false;
            }
            pArgs->append(str);
        } else {
            return false;
        }
    }
    return true;
}

QByteArray OscServer::encodeFloatMessage(const QString& address, float value) {
    QByteArray data;
    appendPaddedString(&data, address);
    appendPaddedString(&data, QStringLiteral(",f"));
    appendFloat32BE(&data, value);
    return data;
}

void OscServer::sendFloat(const QString& address, float value) {
    const QByteArray payload = encodeFloatMessage(address, value);
    m_pSendSocket->writeDatagram(payload, m_sendHost, m_sendPort);
}

bool OscServer::addressToConfigKey(const QString& address, ConfigKey* pKey) const {
    const QStringList parts = address.split('/', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return false;
    }

    if (parts.at(0) == QStringLiteral("deck") && parts.size() >= 3) {
        bool ok = false;
        const int deck = parts.at(1).toInt(&ok);
        if (!ok || deck < 1) {
            return false;
        }
        *pKey = ConfigKey(channelGroupForDeck(deck), parts.at(2));
        return true;
    }

    if (parts.at(0) == QStringLiteral("crossfader")) {
        *pKey = ConfigKey(kMasterGroup, QStringLiteral("crossfader"));
        return true;
    }

    if (parts.at(0) == QStringLiteral("talkover")) {
        *pKey = ConfigKey(kMicrophoneGroup, QStringLiteral("talkover"));
        return true;
    }

    if (parts.size() >= 2 && parts.at(0) == QStringLiteral("microphone") &&
            parts.at(1) == QStringLiteral("gain")) {
        *pKey = ConfigKey(kMicrophoneGroup, QStringLiteral("pregain"));
        return true;
    }

    if (parts.at(0) == QStringLiteral("export") && parts.size() >= 2) {
        *pKey = ConfigKey(kExportGroup, parts.at(1));
        return true;
    }

    if (parts.at(0).startsWith(QStringLiteral("EffectRack")) && parts.size() >= 2) {
        *pKey = ConfigKey(QStringLiteral("[%1]").arg(parts.at(0)), parts.at(1));
        return true;
    }

    return false;
}

QString OscServer::configKeyToAddress(const ConfigKey& key) const {
    static const QRegularExpression channelPattern(QStringLiteral(R"(^\[Channel(\d+)\]$)"));
    const QRegularExpressionMatch match = channelPattern.match(key.group);
    if (match.hasMatch()) {
        return QStringLiteral("/deck/%1/%2").arg(match.captured(1), key.item);
    }
    if (key.group == QLatin1String(kMasterGroup) && key.item == QLatin1String("crossfader")) {
        return QStringLiteral("/crossfader");
    }
    if (key.group == QLatin1String(kMicrophoneGroup) && key.item == QLatin1String("pregain")) {
        return QStringLiteral("/microphone/gain");
    }
    if (key.group == QLatin1String(kMicrophoneGroup) && key.item == QLatin1String("talkover")) {
        return QStringLiteral("/talkover");
    }
    if (key.group.startsWith('[') && key.group.endsWith(']')) {
        const QString bareGroup = key.group.mid(1, key.group.size() - 2);
        return QStringLiteral("/%1/%2").arg(bareGroup, key.item);
    }
    return QString();
}

void OscServer::setupSubscriptions() {
    m_subscriptions.clear();

    static const char* kDeckItems[] = {
            "play",
            "bpm",
            "rate",
            "volume",
            "pregain",
            "sync_enabled",
            "sync_leader",
            "loop_enabled",
            "keylock",
            "quantize",
            "pfl",
            "track_samples",
            "track_samplerate",
    };

    for (int deck = 1; deck <= 4; ++deck) {
        for (const char* item : kDeckItems) {
            const ConfigKey key(channelGroupForDeck(deck), QString::fromLatin1(item));
            const QString address = configKeyToAddress(key);
            if (!address.isEmpty()) {
                subscribeOutbound(key, address);
            }
        }
    }

    subscribeOutbound(ConfigKey(kMasterGroup, QStringLiteral("crossfader")),
            QStringLiteral("/crossfader"));
}

void OscServer::subscribeOutbound(const ConfigKey& key, const QString& outboundAddress) {
    Subscription subscription;
    subscription.pProxy = std::make_unique<ControlProxy>(
            key, this, ControlFlag::AllowMissingOrInvalid);
    if (!subscription.pProxy->valid()) {
        return;
    }
    subscription.outboundAddress = outboundAddress;
    subscription.pProxy->connectValueChanged(
            this,
            [this, outboundAddress](double value) {
                sendFloat(outboundAddress, static_cast<float>(value));
            },
            Qt::QueuedConnection);
    m_subscriptions.push_back(std::move(subscription));
}

void OscServer::handleMessage(const QString& address, const QVector<QVariant>& args) {
    if (address == QStringLiteral("/mixxxxx/ping")) {
        sendFloat(QStringLiteral("/mixxxxx/pong"), 1.0f);
        return;
    }

    const QStringList parts = address.split('/', Qt::SkipEmptyParts);
    if (parts.size() >= 3 && parts.at(0) == QStringLiteral("deck") &&
            parts.at(2) == QStringLiteral("LoadTrack")) {
        bool ok = false;
        const int deck = parts.at(1).toInt(&ok);
        if (ok && deck >= 1 && !args.isEmpty() && m_pPlayerManager != nullptr) {
            const QString path = args.first().toString();
            if (!path.isEmpty()) {
                m_pPlayerManager->slotLoadToDeck(path, deck);
            }
        }
        return;
    }

    ConfigKey key;
    if (!addressToConfigKey(address, &key)) {
        qDebug() << "OSC: unmapped address" << address;
        return;
    }

    if (args.isEmpty()) {
        return;
    }

    const QVariant& arg = args.first();
    if (arg.typeId() == QMetaType::QString) {
        qDebug() << "OSC: string argument ignored for" << address;
        return;
    }

    ControlObject::set(key, arg.toDouble());
}

void OscServer::slotReadPendingDatagrams() {
    while (m_pReceiveSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(m_pReceiveSocket->pendingDatagramSize()));
        m_pReceiveSocket->readDatagram(datagram.data(), datagram.size());

        QString address;
        QVector<QVariant> args;
        if (!decodeMessage(datagram, &address, &args)) {
            qWarning() << "OSC: failed to decode datagram";
            continue;
        }
        handleMessage(address, args);
    }
}

} // namespace mixxx
