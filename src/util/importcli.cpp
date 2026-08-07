#include "util/importcli.h"

#include <QFile>
#include <QFileInfo>
#include <QBuffer>
#include <QtEndian>
#include <QXmlStreamReader>
#include <QtDebug>

#include "library/parser.h"
#include "library/trackcollection.h"
#include "library/trackcollectionmanager.h"
#include "library/trackset/crate/crate.h"
#include "track/track.h"
#include "track/trackid.h"
#include "track/trackref.h"
#include "util/fileinfo.h"

namespace {

constexpr int kSeratoHeaderSize = 2 * static_cast<int>(sizeof(quint32));

quint32 seratoFieldId(const QByteArray& headerData) {
    return qFromBigEndian<quint32>(
            reinterpret_cast<const uchar*>(headerData.constData()));
}

quint32 seratoFieldSize(const QByteArray& headerData) {
    return qFromBigEndian<quint32>(
            reinterpret_cast<const uchar*>(headerData.constData() + sizeof(quint32)));
}

QString seratoUtf16BeToQString(const QByteArray& data) {
    if (data.isEmpty()) {
        return QString();
    }
    QString result;
    result.reserve(data.size() / 2);
    for (int i = 0; i + 1 < data.size(); i += 2) {
        const char16_t ch = static_cast<char16_t>(
                (static_cast<uchar>(data[i]) << 8) |
                static_cast<uchar>(data[i + 1]));
        result.append(QChar(ch));
    }
    return result;
}

QString parseSeratoNestedTrackPath(QIODevice* buffer) {
    QString location;
    QByteArray headerData = buffer->read(kSeratoHeaderSize);
    while (headerData.length() == kSeratoHeaderSize) {
        const quint32 fieldId = seratoFieldId(headerData);
        const quint32 fieldSize = seratoFieldSize(headerData);
        QByteArray data = buffer->read(static_cast<qint64>(fieldSize));
        if (static_cast<quint32>(data.length()) != fieldSize) {
            return QString();
        }

        if (fieldId == 0x7074726b) { // ptrk
            location = seratoUtf16BeToQString(data);
        }

        headerData = buffer->read(kSeratoHeaderSize);
    }
    return location;
}

QList<QString> parseSeratoCrateLocations(const QString& crateFilePath) {
    QList<QString> locations;
    QFile crateFile(crateFilePath);
    if (!crateFile.open(QIODevice::ReadOnly)) {
        qWarning() << "ImportCli: cannot open Serato crate" << crateFilePath;
        return locations;
    }

    QByteArray headerData = crateFile.read(kSeratoHeaderSize);
    while (headerData.length() == kSeratoHeaderSize) {
        const quint32 fieldId = seratoFieldId(headerData);
        const quint32 fieldSize = seratoFieldSize(headerData);
        QByteArray data = crateFile.read(static_cast<qint64>(fieldSize));
        if (static_cast<quint32>(data.length()) != fieldSize) {
            break;
        }

        if (fieldId == 0x6f74726b) { // otrk
            QBuffer buffer(&data);
            buffer.open(QIODevice::ReadOnly);
            const QString location = parseSeratoNestedTrackPath(&buffer);
            if (!location.isEmpty()) {
                locations.append(location);
            }
        }

        headerData = crateFile.read(kSeratoHeaderSize);
    }
    return locations;
}

QList<QString> parseVirtualDjFolderLocations(const QString& vdjFolderPath) {
    QList<QString> locations;
    QFile file(vdjFolderPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "ImportCli: cannot open VirtualDJ folder" << vdjFolderPath;
        return locations;
    }

    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name() == QStringLiteral("track")) {
            const QString location = xml.readElementText().trimmed();
            if (!location.isEmpty()) {
                locations.append(location);
            }
        }
    }
    return locations;
}

QList<QString> parseImportSourceLocations(const QString& sourcePath, QString* pError) {
    const QFileInfo fileInfo(sourcePath);
    if (!fileInfo.exists()) {
        if (pError) {
            *pError = QStringLiteral("Import source does not exist: %1").arg(sourcePath);
        }
        return {};
    }

    const QString suffix = fileInfo.suffix().toLower();
    if (suffix == QStringLiteral("vdjfolder")) {
        return parseVirtualDjFolderLocations(sourcePath);
    }
    if (suffix == QStringLiteral("crate")) {
        return parseSeratoCrateLocations(sourcePath);
    }
    if (Parser::isPlaylistFilenameSupported(sourcePath)) {
        return Parser::parse(sourcePath);
    }

    if (pError) {
        *pError = QStringLiteral(
                "Unsupported import source '%1'. Use M3U/M3U8/PLS/CSV, "
                ".vdjfolder, or Serato .crate.")
                          .arg(sourcePath);
    }
    return {};
}

QString defaultCrateNameFromSource(const QString& sourcePath) {
    return QFileInfo(sourcePath).completeBaseName().trimmed();
}

} // namespace

namespace mixxx {

QStringList ImportCli::parseImportLocations(
        const QString& sourcePath,
        QString* pError) {
    return parseImportSourceLocations(sourcePath, pError);
}

bool ImportCli::executeImportCrate(
        const QString& sourcePath,
        const QString& intoCrateName,
        TrackCollectionManager* pTrackCollectionManager,
        QString* pError) {
    if (sourcePath.trimmed().isEmpty()) {
        if (pError) {
            *pError = QStringLiteral("Import source path must not be empty.");
        }
        return false;
    }
    if (!pTrackCollectionManager) {
        if (pError) {
            *pError = QStringLiteral("Track collection is not initialized.");
        }
        return false;
    }

    QString crateName = intoCrateName.trimmed();
    if (crateName.isEmpty()) {
        crateName = defaultCrateNameFromSource(sourcePath);
    }
    if (crateName.isEmpty()) {
        if (pError) {
            *pError = QStringLiteral(
                    "Crate name is empty; pass --into-crate <name>.");
        }
        return false;
    }

    const QList<QString> locations = parseImportSourceLocations(sourcePath, pError);
    if (locations.isEmpty()) {
        if (!pError || pError->isEmpty()) {
            if (pError) {
                *pError = QStringLiteral("No importable tracks found in %1.")
                                  .arg(sourcePath);
            }
        }
        return false;
    }

    TrackCollection* pCollection = pTrackCollectionManager->internalCollection();
    Crate crate;
    CrateId crateId;
    if (pCollection->crates().readCrateByName(crateName, &crate)) {
        crateId = crate.getId();
    } else {
        crate.setName(crateName);
        if (!pCollection->insertCrate(crate, &crateId)) {
            if (pError) {
                *pError = QStringLiteral("Failed to create crate '%1'.")
                                  .arg(crateName);
            }
            return false;
        }
    }

    QList<TrackId> trackIds;
    trackIds.reserve(locations.size());
    int skipped = 0;
    for (const QString& location : locations) {
        mixxx::FileInfo fileInfo(location);
        if (!fileInfo.checkFileExists()) {
            ++skipped;
            continue;
        }
        const TrackPointer pTrack = pTrackCollectionManager->getOrAddTrack(
                TrackRef::fromFileInfo(fileInfo));
        if (!pTrack) {
            ++skipped;
            continue;
        }
        trackIds.append(pTrack->getId());
    }

    if (trackIds.isEmpty()) {
        if (pError) {
            *pError = QStringLiteral(
                    "No tracks from '%1' exist on disk (%2 entries skipped).")
                              .arg(sourcePath)
                              .arg(skipped);
        }
        return false;
    }

    if (!pCollection->addCrateTracks(crateId, trackIds)) {
        if (pError) {
            *pError = QStringLiteral("Failed to add tracks to crate '%1'.")
                              .arg(crateName);
        }
        return false;
    }

    qInfo().noquote() << QStringLiteral(
            "Imported %1 tracks into crate '%2' from %3 (%4 skipped, missing on disk).")
                                     .arg(trackIds.size())
                                     .arg(crateName)
                                     .arg(sourcePath)
                                     .arg(skipped);
    return true;
}

} // namespace mixxx
