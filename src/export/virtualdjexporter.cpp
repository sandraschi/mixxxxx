#include "export/virtualdjexporter.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QXmlStreamWriter>
#include <QtDebug>

#include "moc_virtualdjexporter.cpp"

namespace {

const QString kVirtualDjFilename = QStringLiteral("database.xml");
const QString kDbConnectionName = QStringLiteral("virtualdj_export_mixxxdb");

} // anonymous namespace

VirtualDjExporter::VirtualDjExporter(const QString& settingsPath, QObject* parent)
        : QObject(parent),
          m_settingsPath(settingsPath) {
}

bool VirtualDjExporter::exportLibrary(const QString& exportPath) {
    emit exportProgress(0, 1);

    QSqlDatabase db = QSqlDatabase::addDatabase(
            QStringLiteral("QSQLITE"), kDbConnectionName);
    db.setDatabaseName(m_settingsPath + QStringLiteral("/mixxxdb.sqlite"));
    if (!db.open()) {
        qWarning() << "VirtualDjExport: Failed to open Mixxx database at"
                    << m_settingsPath;
        emit exportComplete(false, QStringLiteral("Cannot open Mixxx database"));
        return false;
    }

    QSqlQuery trackQuery(db);
    trackQuery.prepare(QStringLiteral(
            "SELECT library.artist, library.title, library.bpm, library.key, "
            "track_locations.location, library.duration, library.genre, "
            "library.year "
            "FROM library "
            "INNER JOIN track_locations ON library.location = track_locations.id "
            "ORDER BY library.artist, library.title"));

    if (!trackQuery.exec()) {
        qWarning() << "VirtualDjExport: Query failed:" << trackQuery.lastError();
        db.close();
        QSqlDatabase::removeDatabase(kDbConnectionName);
        emit exportComplete(false, QStringLiteral("Database query failed"));
        return false;
    }

    QFile file(exportPath + QStringLiteral("/") + kVirtualDjFilename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "VirtualDjExport: Cannot write to"
                    << file.fileName() << file.errorString();
        db.close();
        QSqlDatabase::removeDatabase(kDbConnectionName);
        emit exportComplete(false, QStringLiteral("Cannot write export file"));
        return false;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement(QStringLiteral("database"));

    int count = 0;
    while (trackQuery.next()) {
        xml.writeStartElement(QStringLiteral("song"));

        QString location = trackQuery.value(QStringLiteral("location")).toString();
        xml.writeAttribute(QStringLiteral("FilePath"), location);

        QString artist = trackQuery.value(QStringLiteral("artist")).toString();
        if (!artist.isEmpty()) {
            xml.writeAttribute(QStringLiteral("Artist"), artist);
        }

        QString title = trackQuery.value(QStringLiteral("title")).toString();
        if (!title.isEmpty()) {
            xml.writeAttribute(QStringLiteral("Title"), title);
        }

        double bpm = trackQuery.value(QStringLiteral("bpm")).toDouble();
        if (bpm > 0) {
            xml.writeAttribute(QStringLiteral("BPM"), QString::number(bpm, 'f', 1));
        }

        QString key = convertKeyToCamelot(
                trackQuery.value(QStringLiteral("key")).toString());
        if (!key.isEmpty()) {
            xml.writeAttribute(QStringLiteral("Key"), key);
        }

        int duration = trackQuery.value(QStringLiteral("duration")).toInt();
        if (duration > 0) {
            xml.writeAttribute(QStringLiteral("Length"), QString::number(duration));
        }

        QString genre = trackQuery.value(QStringLiteral("genre")).toString();
        if (!genre.isEmpty()) {
            xml.writeAttribute(QStringLiteral("Genre"), genre);
        }

        xml.writeEndElement(); // song
        count++;
        if (count % 100 == 0) {
            emit exportProgress(count, count + 100);
        }
    }

    xml.writeEndElement(); // database
    xml.writeEndDocument();
    file.close();
    db.close();
    QSqlDatabase::removeDatabase(kDbConnectionName);

    qInfo().noquote()
            << QStringLiteral("VirtualDjExport: Exported %1 tracks to %2")
                       .arg(count)
                       .arg(file.fileName());
    emit exportComplete(true,
            QStringLiteral("Exported %1 tracks to VirtualDJ format").arg(count));
    return true;
}

bool VirtualDjExporter::exportCrate(const QString& crateName, const QString& exportPath) {
    emit exportProgress(0, 1);

    QSqlDatabase db = QSqlDatabase::addDatabase(
            QStringLiteral("QSQLITE"), kDbConnectionName);
    db.setDatabaseName(m_settingsPath + QStringLiteral("/mixxxdb.sqlite"));
    if (!db.open()) {
        qWarning() << "VirtualDjExport: Failed to open Mixxx database";
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

    QStringList trackPaths;
    QSqlQuery tracksQuery(db);
    tracksQuery.prepare(QStringLiteral(
            "SELECT track_locations.location FROM library "
            "INNER JOIN track_locations ON library.location = track_locations.id "
            "INNER JOIN crate_tracks ON crate_tracks.track_id = library.id "
            "WHERE crate_tracks.crate_id = :crateId "
            "ORDER BY crate_tracks.position"));
    tracksQuery.bindValue(QStringLiteral(":crateId"), crateId);

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

    QFile file(exportPath + QStringLiteral("/") + crateName + QStringLiteral(".vdjfolder"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit exportComplete(false,
                QStringLiteral("Cannot write crate file for '%1'").arg(crateName));
        return false;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement(QStringLiteral("playlist"));
    xml.writeAttribute(QStringLiteral("name"), crateName);

    for (const QString& trackPath : trackPaths) {
        xml.writeTextElement(QStringLiteral("track"), trackPath);
    }

    xml.writeEndElement(); // playlist
    xml.writeEndDocument();
    file.close();

    emit exportProgress(trackPaths.size(), trackPaths.size());
    emit exportComplete(true,
            QStringLiteral("Exported %1 tracks from crate '%2' to VirtualDJ")
                    .arg(trackPaths.size())
                    .arg(crateName));
    return true;
}

QString VirtualDjExporter::convertKeyToCamelot(const QString& mixxxKey) const {
    // Mixxx stores keys in Open Key notation or chromatic name.
    // Convert to VirtualDJ Camelot notation.
    static const QMap<QString, QString> keyMap = {
            {QStringLiteral("1A"),  QStringLiteral("8A")},
            {QStringLiteral("1B"),  QStringLiteral("5A")},
            {QStringLiteral("2A"),  QStringLiteral("3A")},
            {QStringLiteral("2B"),  QStringLiteral("12A")},
            {QStringLiteral("3A"),  QStringLiteral("10A")},
            {QStringLiteral("3B"),  QStringLiteral("7A")},
            {QStringLiteral("4A"),  QStringLiteral("5A")},
            {QStringLiteral("4B"),  QStringLiteral("2A")},
            {QStringLiteral("5A"),  QStringLiteral("12A")},
            {QStringLiteral("5B"),  QStringLiteral("9A")},
            {QStringLiteral("6A"),  QStringLiteral("7A")},
            {QStringLiteral("6B"),  QStringLiteral("4A")},
            {QStringLiteral("7A"),  QStringLiteral("2A")},
            {QStringLiteral("7B"),  QStringLiteral("11A")},
            {QStringLiteral("8A"),  QStringLiteral("9A")},
            {QStringLiteral("8B"),  QStringLiteral("6A")},
            {QStringLiteral("9A"),  QStringLiteral("4A")},
            {QStringLiteral("9B"),  QStringLiteral("1A")},
            {QStringLiteral("10A"), QStringLiteral("11A")},
            {QStringLiteral("10B"), QStringLiteral("8A")},
            {QStringLiteral("11A"), QStringLiteral("6A")},
            {QStringLiteral("11B"), QStringLiteral("3A")},
            {QStringLiteral("12A"), QStringLiteral("1A")},
            {QStringLiteral("12B"), QStringLiteral("10A")},
            {QStringLiteral("B"),   QStringLiteral("8B")},
            {QStringLiteral("C"),   QStringLiteral("5B")},
            {QStringLiteral("D"),   QStringLiteral("10B")},
            {QStringLiteral("E"),   QStringLiteral("7B")},
            {QStringLiteral("F"),   QStringLiteral("2B")},
            {QStringLiteral("F#m"), QStringLiteral("11B")},
            {QStringLiteral("A"),   QStringLiteral("8B")},
            {QStringLiteral("Am"),  QStringLiteral("1B")},
            {QStringLiteral("Bm"),  QStringLiteral("3B")},
            {QStringLiteral("Cm"),  QStringLiteral("5B")},
            {QStringLiteral("Dm"),  QStringLiteral("10B")},
            {QStringLiteral("Em"),  QStringLiteral("12B")},
            {QStringLiteral("Fm"),  QStringLiteral("7B")},
            {QStringLiteral("Gm"),  QStringLiteral("2B")},
    };

    return keyMap.value(mixxxKey.toUpper(), QString());
}
