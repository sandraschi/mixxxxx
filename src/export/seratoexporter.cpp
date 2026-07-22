#include "export/seratoexporter.h"

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QtDebug>
#include <QtEndian>

#include "moc_seratoexporter.cpp"

namespace {

const QString kSeratoSubdir = QStringLiteral("_Serato_");
const QString kDatabaseFilename = QStringLiteral("database V2");
const QString kSubcrateDir = QStringLiteral("Subcrates");
const QString kDbConnectionName = QStringLiteral("serato_export_mixxxdb");

QByteArray seratoField(quint32 fieldId, const QByteArray& value) {
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << fieldId;
    stream << static_cast<quint32>(value.size());
    stream.writeRawData(value.constData(), value.size());
    return data;
}

QByteArray seratoStringField(quint32 fieldId, const QString& value) {
    if (value.isEmpty()) {
        return seratoField(fieldId, QByteArray());
    }
    const auto* data = reinterpret_cast<const quint16*>(value.utf16());
    int len = value.length();
    QByteArray bigEndianUtf16(len * 2, '\0');
    for (int i = 0; i < len; ++i) {
        bigEndianUtf16[i * 2] = static_cast<char>((data[i] >> 8) & 0xFF);
        bigEndianUtf16[i * 2 + 1] = static_cast<char>(data[i] & 0xFF);
    }
    return seratoField(fieldId, bigEndianUtf16);
}

QByteArray seratoBoolField(quint32 fieldId, bool value) {
    QByteArray boolData(1, value ? 1 : 0);
    return seratoField(fieldId, boolData);
}

QByteArray makeTrackEntry(const QString& location,
        const QString& artist,
        const QString& title,
        const QString& album,
        const QString& genre,
        const QString& comment,
        const QString& grouping,
        const QString& label,
        const QString& key,
        const QString& duration,
        const QString& bitrate,
        const QString& samplerate,
        const QString& bpm,
        int year,
        bool beatgridLocked) {
    static constexpr quint32 kFieldIdTrack = 0x6f74726b;   // otrk
    static constexpr quint32 kFieldIdFileType = 0x74747970; // ttyp
    static constexpr quint32 kFieldIdFilePath = 0x7066696c; // pfil
    static constexpr quint32 kFieldIdTitle = 0x74736e67;    // tsng
    static constexpr quint32 kFieldIdArtist = 0x74617274;   // tart
    static constexpr quint32 kFieldIdAlbum = 0x74616c62;    // talb
    static constexpr quint32 kFieldIdGenre = 0x7467656e;    // tgen
    static constexpr quint32 kFieldIdComment = 0x74636f6d;  // tcom
    static constexpr quint32 kFieldIdGrouping = 0x74677270; // tgrp
    static constexpr quint32 kFieldIdLabel = 0x746c626c;    // tlbl
    static constexpr quint32 kFieldIdYear = 0x74747972;     // ttyr
    static constexpr quint32 kFieldIdLength = 0x746c656e;   // tlen
    static constexpr quint32 kFieldIdBitrate = 0x74626974;  // tbit
    static constexpr quint32 kFieldIdSampleRate = 0x74736d70; // tsmp
    static constexpr quint32 kFieldIdBpm = 0x7462706d;      // tbpm
    static constexpr quint32 kFieldIdKey = 0x746b6579;      // tkey
    static constexpr quint32 kFieldIdBeatgridLocked = 0x6262676c; // bbgl

    QByteArray trackData;
    trackData.append(seratoStringField(kFieldIdFileType, QFileInfo(location).suffix()));
    trackData.append(seratoStringField(kFieldIdFilePath, location));
    trackData.append(seratoStringField(kFieldIdTitle, title));
    trackData.append(seratoStringField(kFieldIdArtist, artist));
    trackData.append(seratoStringField(kFieldIdAlbum, album));
    trackData.append(seratoStringField(kFieldIdGenre, genre));
    trackData.append(seratoStringField(kFieldIdComment, comment));
    trackData.append(seratoStringField(kFieldIdGrouping, grouping));
    trackData.append(seratoStringField(kFieldIdLabel, label));
    if (year > 0) {
        trackData.append(seratoStringField(kFieldIdYear, QString::number(year)));
    }
    trackData.append(seratoStringField(kFieldIdLength, duration));
    trackData.append(seratoStringField(kFieldIdBitrate, bitrate));
    trackData.append(seratoStringField(kFieldIdSampleRate, samplerate));
    trackData.append(seratoStringField(kFieldIdBpm, bpm));
    trackData.append(seratoStringField(kFieldIdKey, key));
    trackData.append(seratoBoolField(kFieldIdBeatgridLocked, beatgridLocked));

    return seratoField(kFieldIdTrack, trackData);
}

QByteArray makeCrateTrackEntry(const QString& location) {
    static constexpr quint32 kFieldIdTrack = 0x6f74726b;   // otrk
    static constexpr quint32 kFieldIdTrackPath = 0x7074726b; // ptrk

    QByteArray trackData;
    trackData.append(seratoStringField(kFieldIdTrackPath, location));
    return seratoField(kFieldIdTrack, trackData);
}

} // anonymous namespace

SeratoExporter::SeratoExporter(const QString& settingsPath, QObject* parent)
        : QObject(parent),
          m_settingsPath(settingsPath) {
}

bool SeratoExporter::exportLibrary(const QString& exportPath) {
    emit exportProgress(0, 1);

    QSqlDatabase db = QSqlDatabase::addDatabase(
            QStringLiteral("QSQLITE"), kDbConnectionName);
    db.setDatabaseName(m_settingsPath + QStringLiteral("/mixxxdb.sqlite"));
    if (!db.open()) {
        qWarning() << "SeratoExport: Failed to open Mixxx database at"
                    << m_settingsPath;
        emit exportComplete(false, QStringLiteral("Cannot open Mixxx database"));
        return false;
    }

    QSqlQuery trackQuery(db);
    trackQuery.prepare(QStringLiteral(
            "SELECT library.id, library.artist, library.title, library.album, "
            "library.genre, library.year, library.composer, library.comment, "
            "library.bpm, library.duration, library.samplerate, library.bitrate, "
            "library.channels, library.rating, library.key, library.cuepoint, "
            "library.tracknumber, library.filetype, library.bpm_lock, "
            "library.grouping, track_locations.location "
            "FROM library "
            "INNER JOIN track_locations ON library.location = track_locations.id "
            "ORDER BY library.id"));

    if (!trackQuery.exec()) {
        qWarning() << "SeratoExport: Query failed:" << trackQuery.lastError();
        db.close();
        QSqlDatabase::removeDatabase(kDbConnectionName);
        emit exportComplete(false, QStringLiteral("Database query failed"));
        return false;
    }

    QDir exportDir(exportPath);
    QString seratoDirPath = exportDir.filePath(kSeratoSubdir);
    QDir seratoDir(seratoDirPath);
    if (!seratoDir.exists()) {
        exportDir.mkpath(kSeratoSubdir);
    }

    int totalTracks = 0;
    while (trackQuery.next()) {
        totalTracks++;
    }
    trackQuery.seek(-1);

    // Build the database V2 file
    static constexpr quint32 kFieldIdVersion = 0x7672736e; // vrsn
    static constexpr quint32 kFieldIdTrack = 0x6f74726b;   // otrk

    QByteArray databaseData;
    databaseData.append(seratoStringField(kFieldIdVersion,
            QStringLiteral("2.0/Serato DJ Pro/2.0.0")));

    int trackIndex = 0;
    QMap<QString, int> trackPathToId;
    while (trackQuery.next()) {
        emit exportProgress(trackIndex + 1, totalTracks);

        int trackId = trackQuery.value(QStringLiteral("id")).toInt();
        QString artist = trackQuery.value(QStringLiteral("artist")).toString();
        QString title = trackQuery.value(QStringLiteral("title")).toString();
        QString album = trackQuery.value(QStringLiteral("album")).toString();
        QString genre = trackQuery.value(QStringLiteral("genre")).toString();
        QString comment = trackQuery.value(QStringLiteral("comment")).toString();
        QString grouping = trackQuery.value(QStringLiteral("grouping")).toString();
        int year = trackQuery.value(QStringLiteral("year")).toInt();
        double bpm = trackQuery.value(QStringLiteral("bpm")).toDouble();
        double durationSecs = trackQuery.value(QStringLiteral("duration")).toDouble();
        double sampleRate = trackQuery.value(QStringLiteral("samplerate")).toDouble();
        int bitrate = trackQuery.value(QStringLiteral("bitrate")).toInt();
        int mixxxKey = trackQuery.value(QStringLiteral("key")).toInt();
        QString filetype = trackQuery.value(QStringLiteral("filetype")).toString();
        bool bpmLock = trackQuery.value(QStringLiteral("bpm_lock")).toBool();

        QString location = QFileInfo(
                trackQuery.value(QStringLiteral("location")).toString())
                                   .absoluteFilePath();

        QString locationRelative;
        if (location.startsWith(exportDir.absolutePath(), Qt::CaseInsensitive)) {
            locationRelative = exportDir.relativeFilePath(location);
        } else {
            locationRelative = location;
        }

        QByteArray trackEntry = makeTrackEntry(
                locationRelative,
                artist,
                title,
                album,
                genre,
                comment,
                grouping,
                QString(), // label
                QString(), // key text
                durationSecs > 0
                        ? QString::number(static_cast<int>(durationSecs))
                        : QString(),
                bitrate > 0 ? QString::number(bitrate) : QString(),
                sampleRate > 0 ? QString::number(static_cast<int>(sampleRate))
                               : QString(),
                bpm > 0 ? QString::number(bpm, 'f', 1) : QString(),
                year,
                bpmLock);
        databaseData.append(trackEntry);

        trackPathToId.insert(locationRelative, trackIndex);
        trackIndex++;
    }

    QFile databaseFile(seratoDir.filePath(kDatabaseFilename));
    if (!databaseFile.open(QIODevice::WriteOnly)) {
        qWarning() << "SeratoExport: Cannot write database file:"
                    << databaseFile.errorString();
        db.close();
        QSqlDatabase::removeDatabase(kDbConnectionName);
        emit exportComplete(false,
                QStringLiteral("Cannot write database V2 file"));
        return false;
    }
    databaseFile.write(databaseData);
    databaseFile.close();

    // Export crates
    QSqlQuery crateQuery(db);
    crateQuery.prepare(QStringLiteral(
            "SELECT id, name FROM crates ORDER BY name"));
    if (crateQuery.exec()) {
        QDir subcrateDir(seratoDir.filePath(kSubcrateDir));
        if (!subcrateDir.exists()) {
            seratoDir.mkpath(kSubcrateDir);
        }

        while (crateQuery.next()) {
            int crateId = crateQuery.value(QStringLiteral("id")).toInt();
            QString crateName = crateQuery.value(QStringLiteral("name")).toString();

            QSqlQuery tracksQuery(db);
            tracksQuery.prepare(QStringLiteral(
                    "SELECT track_locations.location FROM library "
                    "INNER JOIN track_locations ON library.location = track_locations.id "
                    "INNER JOIN crate_tracks ON crate_tracks.track_id = library.id "
                    "WHERE crate_tracks.crate_id = :crateId "
                    "ORDER BY crate_tracks.position"));
            tracksQuery.bindValue(QStringLiteral(":crateId"), crateId);

            QByteArray crateData;
            crateData.append(seratoStringField(kFieldIdVersion,
                    QStringLiteral("2.0/Serato DJ Pro/2.0.0")));

            if (tracksQuery.exec()) {
                while (tracksQuery.next()) {
                    QString trackLocation =
                            tracksQuery.value(QStringLiteral("location"))
                                    .toString();
                    QString locationRelative;
                    if (trackLocation.startsWith(
                                exportDir.absolutePath(), Qt::CaseInsensitive)) {
                        locationRelative =
                                exportDir.relativeFilePath(trackLocation);
                    } else {
                        locationRelative = trackLocation;
                    }
                    crateData.append(makeCrateTrackEntry(locationRelative));
                }
            }

            QFile crateFile(subcrateDir.filePath(crateName + QStringLiteral(".crate")));
            if (crateFile.open(QIODevice::WriteOnly)) {
                crateFile.write(crateData);
                crateFile.close();
            }
        }
    }

    db.close();
    QSqlDatabase::removeDatabase(kDbConnectionName);

    // Write README
    QFile readmeFile(seratoDir.filePath(QStringLiteral("README.txt")));
    if (readmeFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        readmeFile.write(
                QStringLiteral(
                        "Serato Library exported from Mixxx\n"
                        "==================================\n"
                        "\n"
                        "This directory contains a Serato-compatible database V2 file\n"
                        "and crate data.\n"
                        "\n"
                        "The track database (_Serato_/database V2) contains all Mixxx\n"
                        "library tracks with metadata (artist, title, album, BPM, etc.).\n"
                        "\n"
                        "Crates are exported as .crate files in _Serato_/Subcrates/.\n"
                        "\n"
                        "NOTE: Cue points and beatgrids are NOT written to this export.\n"
                        "Serato reads cue points and beatgrids from file metadata tags\n"
                        "(Serato Markers_ and SeratoBeatGrid in MP3/FLAC/AAC tags).\n"
                        "To transfer cues/beatgrids, use Mixxx's built-in tag writing\n"
                        "to embed them in the audio files before exporting.\n"
                        "\n"
                        "Export date: %1\n")
                        .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
                        .toUtf8());
        readmeFile.close();
    }

    qInfo().noquote()
            << QStringLiteral("SeratoExport: Exported %1 tracks to %2")
                       .arg(trackIndex)
                       .arg(seratoDirPath);
    emit exportComplete(true,
            QStringLiteral("Exported %1 tracks to Serato format")
                    .arg(trackIndex));
    return true;
}

bool SeratoExporter::exportCrate(const QString& crateName, const QString& exportPath) {
    emit exportProgress(0, 1);

    QSqlDatabase db = QSqlDatabase::addDatabase(
            QStringLiteral("QSQLITE"), kDbConnectionName);
    db.setDatabaseName(m_settingsPath + QStringLiteral("/mixxxdb.sqlite"));
    if (!db.open()) {
        qWarning() << "SeratoExport: Failed to open Mixxx database";
        emit exportComplete(false, QStringLiteral("Cannot open Mixxx database"));
        return false;
    }

    QSqlQuery crateQuery(db);
    crateQuery.prepare(QStringLiteral("SELECT id FROM crates WHERE name = :name"));
    crateQuery.bindValue(QStringLiteral(":name"), crateName);
    if (!crateQuery.exec() || !crateQuery.next()) {
        db.close();
        QSqlDatabase::removeDatabase(kDbConnectionName);
        emit exportComplete(false,
                QStringLiteral("Crate '%1' not found").arg(crateName));
        return false;
    }
    int crateId = crateQuery.value(QStringLiteral("id")).toInt();

    QSqlQuery tracksQuery(db);
    tracksQuery.prepare(QStringLiteral(
            "SELECT track_locations.location FROM library "
            "INNER JOIN track_locations ON library.location = track_locations.id "
            "INNER JOIN crate_tracks ON crate_tracks.track_id = library.id "
            "WHERE crate_tracks.crate_id = :crateId "
            "ORDER BY crate_tracks.position"));
    tracksQuery.bindValue(QStringLiteral(":crateId"), crateId);

    QStringList trackPaths;
    if (tracksQuery.exec()) {
        while (tracksQuery.next()) {
            trackPaths.append(
                    tracksQuery.value(QStringLiteral("location")).toString());
        }
    }
    db.close();
    QSqlDatabase::removeDatabase(kDbConnectionName);

    if (trackPaths.isEmpty()) {
        emit exportComplete(false,
                QStringLiteral("Crate '%1' is empty").arg(crateName));
        return false;
    }

    // Ensure directory structure exists
    QDir exportDir(exportPath);
    QDir seratoDir(exportDir.filePath(kSeratoSubdir));
    if (!seratoDir.exists()) {
        exportDir.mkpath(kSeratoSubdir);
    }
    QDir subcrateDir(seratoDir.filePath(kSubcrateDir));
    if (!subcrateDir.exists()) {
        seratoDir.mkpath(kSubcrateDir);
    }

    // Write crate file
    static constexpr quint32 kFieldIdVersion = 0x7672736e; // vrsn

    QByteArray crateData;
    crateData.append(seratoStringField(kFieldIdVersion,
            QStringLiteral("2.0/Serato DJ Pro/2.0.0")));
    for (const QString& trackPath : trackPaths) {
        QString locationRelative;
        if (trackPath.startsWith(exportDir.absolutePath(), Qt::CaseInsensitive)) {
            locationRelative = exportDir.relativeFilePath(trackPath);
        } else {
            locationRelative = trackPath;
        }
        crateData.append(makeCrateTrackEntry(locationRelative));
    }

    QFile crateFile(subcrateDir.filePath(crateName + QStringLiteral(".crate")));
    if (!crateFile.open(QIODevice::WriteOnly)) {
        emit exportComplete(false,
                QStringLiteral("Cannot write crate file for '%1'").arg(crateName));
        return false;
    }
    crateFile.write(crateData);
    crateFile.close();

    emit exportProgress(trackPaths.size(), trackPaths.size());
    emit exportComplete(true,
            QStringLiteral("Exported %1 tracks from crate '%2' to Serato")
                    .arg(trackPaths.size())
                    .arg(crateName));
    return true;
}
