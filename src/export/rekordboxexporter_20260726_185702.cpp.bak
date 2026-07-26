#include "export/rekordboxexporter.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QtDebug>
#include <djinterop/djinterop.hpp>
#include <djinterop/engine/engine.hpp>
#include <djinterop/engine/engine_schema.hpp>

#include "moc_rekordboxexporter.cpp"

namespace e = djinterop::engine;

namespace {

constexpr int kMaxHotCues = 8;

const QString kPioneerSubdir = QStringLiteral("PIONEER/rekordbox");
const QString kDbConnectionName = QStringLiteral("rekordbox_export_mixxxdb");

std::optional<djinterop::musical_key> toDjinteropKey(int mixxxKey) {
    // Map from Mixxx chromatic key id (1-24) to libdjinterop Circle-of-Fifths enum.
    // Returns std::nullopt for G# minor (21) which has no libdjinterop equivalent.
    switch (mixxxKey) {
    case 1:  return djinterop::musical_key::c_major;        // C
    case 2:  return djinterop::musical_key::d_flat_major;   // C#/Db
    case 3:  return djinterop::musical_key::d_major;         // D
    case 4:  return djinterop::musical_key::e_flat_major;    // D#/Eb
    case 5:  return djinterop::musical_key::e_major;         // E
    case 6:  return djinterop::musical_key::f_major;         // F
    case 7:  return djinterop::musical_key::f_sharp_major;   // F#/Gb
    case 8:  return djinterop::musical_key::g_major;         // G
    case 9:  return djinterop::musical_key::a_flat_major;    // G#/Ab
    case 10: return djinterop::musical_key::a_major;         // A
    case 11: return djinterop::musical_key::b_flat_major;    // A#/Bb
    case 12: return djinterop::musical_key::b_major;         // B
    case 13: return djinterop::musical_key::c_minor;         // Cm
    case 14: return djinterop::musical_key::d_flat_minor;    // C#m/Dbm
    case 15: return djinterop::musical_key::d_minor;         // Dm
    case 16: return djinterop::musical_key::e_flat_minor;    // D#m/Ebm
    case 17: return djinterop::musical_key::e_minor;         // Em
    case 18: return djinterop::musical_key::f_minor;         // Fm
    case 19: return djinterop::musical_key::f_sharp_minor;   // F#m/Gbm
    case 20: return djinterop::musical_key::g_minor;         // Gm
    case 21: return std::nullopt;                            // G#m — not in libdjinterop
    case 22: return djinterop::musical_key::a_minor;         // Am
    case 23: return djinterop::musical_key::b_flat_minor;    // A#m/Bbm
    case 24: return djinterop::musical_key::b_minor;         // Bm
    default: return std::nullopt;
    }
}

} // anonymous namespace

RekordboxExporter::RekordboxExporter(const QString& settingsPath, QObject* parent)
    : QObject(parent),
      m_settingsPath(settingsPath) {
}

bool RekordboxExporter::exportTrack(const QString& trackPath, const QString& usbPath) {
    emit exportProgress(0, 1);

    QSqlDatabase db = QSqlDatabase::addDatabase(
            QStringLiteral("QSQLITE"), kDbConnectionName);
    db.setDatabaseName(m_settingsPath + QStringLiteral("/mixxxdb.sqlite"));
    if (!db.open()) {
        qWarning() << "RekordboxExport: Failed to open Mixxx database at"
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
            "library.tracknumber, library.filetype "
            "FROM library "
            "INNER JOIN track_locations ON library.location = track_locations.id "
            "WHERE track_locations.location = :path"));
    trackQuery.bindValue(QStringLiteral(":path"), trackPath);
    if (!trackQuery.exec() || !trackQuery.next()) {
        qWarning() << "RekordboxExport: Track not found:" << trackPath;
        db.close();
        QSqlDatabase::removeDatabase(kDbConnectionName);
        emit exportComplete(false, QStringLiteral("Track not found in library"));
        return false;
    }

    int trackId = trackQuery.value(QStringLiteral("id")).toInt();
    QString artist = trackQuery.value(QStringLiteral("artist")).toString();
    QString title = trackQuery.value(QStringLiteral("title")).toString();
    QString album = trackQuery.value(QStringLiteral("album")).toString();
    QString genre = trackQuery.value(QStringLiteral("genre")).toString();
    QString comment = trackQuery.value(QStringLiteral("comment")).toString();
    QString composer = trackQuery.value(QStringLiteral("composer")).toString();
    int year = trackQuery.value(QStringLiteral("year")).toInt();
    double bpm = trackQuery.value(QStringLiteral("bpm")).toDouble();
    double durationSecs = trackQuery.value(QStringLiteral("duration")).toDouble();
    double sampleRate = trackQuery.value(QStringLiteral("samplerate")).toDouble();
    int bitrate = trackQuery.value(QStringLiteral("bitrate")).toInt();
    int rating = trackQuery.value(QStringLiteral("rating")).toInt();
    int mixxxKey = trackQuery.value(QStringLiteral("key")).toInt();
    double mainCuePos = trackQuery.value(QStringLiteral("cuepoint")).toDouble();
    QString trackNumber = trackQuery.value(QStringLiteral("tracknumber")).toString();

    struct CueRow {
        int type;
        double position;
        double length;
        int hotcue;
        QString label;
        int color;
    };
    QList<CueRow> cueRows;
    QSqlQuery cueQuery(db);
    cueQuery.prepare(QStringLiteral(
            "SELECT type, position, length, hotcue, label, color "
            "FROM cues WHERE track_id = :trackId ORDER BY hotcue"));
    cueQuery.bindValue(QStringLiteral(":trackId"), trackId);
    if (cueQuery.exec()) {
        while (cueQuery.next()) {
            cueRows.append({
                    cueQuery.value(QStringLiteral("type")).toInt(),
                    cueQuery.value(QStringLiteral("position")).toDouble(),
                    cueQuery.value(QStringLiteral("length")).toDouble(),
                    cueQuery.value(QStringLiteral("hotcue")).toInt(),
                    cueQuery.value(QStringLiteral("label")).toString(),
                    cueQuery.value(QStringLiteral("color")).toInt()});
        }
    }

    db.close();
    QSqlDatabase::removeDatabase(kDbConnectionName);

    QDir pioneerDir(usbPath + QStringLiteral("/") + kPioneerSubdir);
    if (!pioneerDir.exists()) {
        QDir().mkpath(pioneerDir.absolutePath());
    }

    try {
        auto engDb = e::create_database(
                pioneerDir.absolutePath().toStdString(),
                e::latest_schema);

        auto snapshot = djinterop::track_snapshot{};
        snapshot.artist = artist.toStdString();
        snapshot.title = title.toStdString();
        snapshot.album = album.toStdString();
        snapshot.genre = genre.toStdString();
        snapshot.comment = comment.toStdString();
        snapshot.composer = composer.toStdString();
        if (!trackNumber.isEmpty()) {
            snapshot.track_number = trackNumber.toInt();
        }
        snapshot.duration = std::chrono::milliseconds(
                static_cast<int64_t>(1000 * durationSecs));
        if (bpm > 0) {
            snapshot.bpm = bpm;
        }
        snapshot.year = year;
        snapshot.bitrate = bitrate;
        snapshot.rating = rating * 20;
        if (sampleRate > 0) {
            snapshot.sample_rate = sampleRate;
            snapshot.sample_count = static_cast<uint64_t>(durationSecs * sampleRate);
        }
        snapshot.key = toDjinteropKey(mixxxKey);
        snapshot.main_cue = mainCuePos > 0 ? mainCuePos : 0.0;

        // Build beatgrid from average BPM
        if (bpm > 0 && sampleRate > 0 && durationSecs > 0) {
            double frameCount = durationSecs * sampleRate;
            double beatFrames = 60.0 * sampleRate / bpm;
            double firstBeat = snapshot.main_cue.value_or(0);
            // Walk backwards in 4-beat steps to find bar-aligned first beat
            while (firstBeat >= beatFrames * 4) {
                firstBeat -= beatFrames * 4;
            }
            int numBeats = static_cast<int>((frameCount - firstBeat) / beatFrames);
            if (numBeats > 0) {
                double lastBeat = firstBeat + numBeats * beatFrames;
                std::vector<djinterop::beatgrid_marker> beatgrid{
                        {0, firstBeat},
                        {numBeats, lastBeat}};
                beatgrid = e::normalize_beatgrid(
                        std::move(beatgrid),
                        static_cast<int64_t>(frameCount));
                snapshot.beatgrid = beatgrid;
            }
        }

        // Hot cues and loops
        snapshot.hot_cues.resize(kMaxHotCues);
        for (const auto& cr : cueRows) {
            if (cr.type == 2 && cr.hotcue >= 0 && cr.hotcue < kMaxHotCues) {
                djinterop::hot_cue hc{};
                hc.label = cr.label.isEmpty()
                        ? QString(QStringLiteral("Cue %1")).arg(cr.hotcue + 1).toStdString()
                        : cr.label.toStdString();
                hc.sample_offset = cr.position;
                snapshot.hot_cues[cr.hotcue] = hc;
            }
        }

        engDb.create_track(snapshot);

        qInfo().noquote() << QStringLiteral("RekordboxExport: Exported %1 - %2")
                                     .arg(artist, title);
        emit exportComplete(true,
                QStringLiteral("Exported: %1 - %2").arg(artist, title));
        return true;

    } catch (const std::exception& e) {
        qWarning() << "RekordboxExport: libdjinterop error:" << e.what();
        emit exportComplete(false,
                QStringLiteral("Export failed: %1").arg(e.what()));
        return false;
    }
}

bool RekordboxExporter::exportCrate(const QString& crateName, const QString& usbPath) {
    QSqlDatabase db = QSqlDatabase::addDatabase(
            QStringLiteral("QSQLITE"), kDbConnectionName);
    db.setDatabaseName(m_settingsPath + QStringLiteral("/mixxxdb.sqlite"));
    if (!db.open()) {
        qWarning() << "RekordboxExport: Failed to open Mixxx database";
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
            "WHERE crate_tracks.crate_id = :crateId"));
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

    int total = trackPaths.size();
    for (int i = 0; i < total; ++i) {
        emit exportProgress(i + 1, total);
        if (!exportTrack(trackPaths[i], usbPath)) {
            qWarning().noquote()
                    << QStringLiteral("RekordboxExport: Failed on %1")
                               .arg(trackPaths[i]);
        }
    }

    emit exportComplete(true,
            QStringLiteral("Exported %1 tracks from crate '%2'")
                    .arg(total)
                    .arg(crateName));
    return true;
}
