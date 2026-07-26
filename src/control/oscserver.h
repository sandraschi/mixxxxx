#pragma once

#include <memory>
#include <vector>

#include <QByteArray>
#include <QHostAddress>
#include <QObject>
#include <QString>
#include <QVector>
#include <QVariant>

#include "preferences/configobject.h"
#include "preferences/usersettings.h"

class PlayerManager;
class QUdpSocket;
class ControlProxy;

namespace mixxx {

class OscServer : public QObject {
    Q_OBJECT

  public:
    static constexpr int kDefaultPortIn = 11119;
    static constexpr int kDefaultPortOut = 11118;

    OscServer(PlayerManager* pPlayerManager,
            UserSettingsPointer pConfig,
            QObject* pParent = nullptr);
    ~OscServer() override;

    void start();
    void stop();

    bool isRunning() const {
        return m_running;
    }

    static bool decodeMessage(const QByteArray& datagram,
            QString* pAddress,
            QVector<QVariant>* pArgs);

    static QByteArray encodeFloatMessage(const QString& address, float value);

  private slots:
    void slotReadPendingDatagrams();

  private:
    struct Subscription {
        std::unique_ptr<ControlProxy> pProxy;
        QString outboundAddress;
    };

    void handleMessage(const QString& address, const QVector<QVariant>& args);
    bool addressToConfigKey(const QString& address, ConfigKey* pKey) const;
    QString configKeyToAddress(const ConfigKey& key) const;
    void sendFloat(const QString& address, float value);
    void subscribeOutbound(const ConfigKey& key, const QString& outboundAddress);
    void setupSubscriptions();

    PlayerManager* m_pPlayerManager;
    UserSettingsPointer m_pConfig;
    QUdpSocket* m_pReceiveSocket;
    QUdpSocket* m_pSendSocket;
    QHostAddress m_sendHost;
    quint16 m_sendPort;
    bool m_running;
    std::vector<Subscription> m_subscriptions;
};

} // namespace mixxx
