#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QList>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QTemporaryDir>
#include <djinterop/djinterop.hpp>
#include <djinterop/engine/engine.hpp>

#include "export/engineexporter.h"
#include "test/mixxxtest.h"
#include "track/cueinfo.h"

// Covers docs/TODO.md items 13, 14, 15 and 17: the exporter's output location
// and naming (13), hot cues exported with the right cue type (14), the engine
// database opened once per crate rather than once per track (15), and the
// missing exporter test itself (17).
//
// Each test seeds a throwaway mixxxdb.sqlite with just the tables and columns
// EngineExporter queries, points the exporter at it, and reads the produced
// Engine Library back through libdjinterop - so these assert on what a
// consumer of the export would actually see, not on internal calls.

namespace {

constexpr auto kSeedConnection = "engineexporter_test_seed";

struct SeedCue {
    int type;
    int hotcue;
    double position;
    QString label;
};

struct SeedTrack {
    int id;
    QString path;
    QString title;
    QList<SeedCue> cues;
};

struct SeedCrate {
    QString name;
    QList<int> trackIds;
};

void execOrFail(QSqlQuery& q, const QString& sql) {
    ASSERT_TRUE(q.exec(sql)) << sql.toStdString() << ": "
                             << q.lastError().text().toStdString();
}

void writeMixxxDb(const QString& settingsDir,
        const QList<SeedTrack>& tracks,
        const QList<SeedCrate>& crates) {
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(
                QStringLiteral("QSQLITE"), QString::fromLatin1(kSeedConnection));
        db.setDatabaseName(settingsDir + QStringLiteral("/mixxxdb.sqlite"));
        ASSERT_TRUE(db.open());

        QSqlQuery q(db);
        execOrFail(q, QStringLiteral("CREATE TABLE track_locations (id INTEGER PRIMARY KEY, location TEXT)"));
        execOrFail(q,
                QStringLiteral("CREATE TABLE library (id INTEGER PRIMARY KEY, artist TEXT, title TEXT, "
                               "album TEXT, genre TEXT, year TEXT, composer TEXT, comment TEXT, "
                               "bpm REAL, duration REAL, samplerate INTEGER, bitrate INTEGER, "
                               "channels INTEGER, rating INTEGER, key INTEGER, cuepoint REAL, "
                               "tracknumber TEXT, filetype TEXT, location INTEGER)"));
        execOrFail(q,
                QStringLiteral("CREATE TABLE cues (id INTEGER PRIMARY KEY, track_id INTEGER, type INTEGER, "
                               "position REAL, length REAL, hotcue INTEGER, label TEXT, color INTEGER)"));
        execOrFail(q, QStringLiteral("CREATE TABLE crates (id INTEGER PRIMARY KEY, name TEXT)"));
        execOrFail(q, QStringLiteral("CREATE TABLE crate_tracks (crate_id INTEGER, track_id INTEGER)"));

        for (const SeedTrack& t : tracks) {
            q.prepare(QStringLiteral("INSERT INTO track_locations (id, location) VALUES (?, ?)"));
            q.addBindValue(t.id);
            q.addBindValue(t.path);
            ASSERT_TRUE(q.exec());

            q.prepare(QStringLiteral(
                    "INSERT INTO library (id, artist, title, album, genre, year, composer, comment, "
                    "bpm, duration, samplerate, bitrate, channels, rating, key, cuepoint, tracknumber, "
                    "filetype, location) VALUES (?, 'Artist', ?, 'Album', 'Genre', '2026', 'Composer', "
                    "'', 128.0, 200.0, 44100, 320, 2, 3, 1, 0.0, '1', 'mp3', ?)"));
            q.addBindValue(t.id);
            q.addBindValue(t.title);
            q.addBindValue(t.id);
            ASSERT_TRUE(q.exec());

            for (const SeedCue& c : t.cues) {
                q.prepare(QStringLiteral(
                        "INSERT INTO cues (track_id, type, position, length, hotcue, label, color) "
                        "VALUES (?, ?, ?, 0.0, ?, ?, 0)"));
                q.addBindValue(t.id);
                q.addBindValue(c.type);
                q.addBindValue(c.position);
                q.addBindValue(c.hotcue);
                q.addBindValue(c.label);
                ASSERT_TRUE(q.exec());
            }
        }

        int crateId = 1;
        for (const SeedCrate& c : crates) {
            q.prepare(QStringLiteral("INSERT INTO crates (id, name) VALUES (?, ?)"));
            q.addBindValue(crateId);
            q.addBindValue(c.name);
            ASSERT_TRUE(q.exec());
            for (int trackId : c.trackIds) {
                q.prepare(QStringLiteral("INSERT INTO crate_tracks (crate_id, track_id) VALUES (?, ?)"));
                q.addBindValue(crateId);
                q.addBindValue(trackId);
                ASSERT_TRUE(q.exec());
            }
            ++crateId;
        }
        db.close();
    }
    // The exporter opens this same file under its own connection name, so the
    // seeding connection must be fully gone first.
    QSqlDatabase::removeDatabase(QString::fromLatin1(kSeedConnection));
}

class EngineExporterTest : public MixxxTest {
  protected:
    QTemporaryDir m_settingsDir;
    QTemporaryDir m_usbDir;
    QTemporaryDir m_musicDir;

    // A real (tiny) file, so the exporter's audio copy has something to copy.
    QString makeAudio(const QString& name) {
        const QString path = m_musicDir.path() + QStringLiteral("/") + name;
        QFile f(path);
        EXPECT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("not really audio");
        f.close();
        return path;
    }

    QString settingsPath() const {
        return m_settingsDir.path();
    }
    QString usbPath() const {
        return m_usbDir.path();
    }
    QString engineLibraryPath() const {
        return m_usbDir.path() + QStringLiteral("/Engine Library");
    }
};

} // namespace

// TODO 13: output goes into "Engine Library" (what libdjinterop actually
// writes), not the old "PIONEER/rekordbox" that no CDJ would read.
TEST_F(EngineExporterTest, ExportTrackWritesEngineLibraryNotPioneer) {
    ASSERT_TRUE(m_settingsDir.isValid());
    ASSERT_TRUE(m_usbDir.isValid());
    const QString audio = makeAudio(QStringLiteral("one.mp3"));
    writeMixxxDb(settingsPath(), {{1, audio, QStringLiteral("One"), {}}}, {});

    EngineExporter exporter(settingsPath());
    ASSERT_TRUE(exporter.exportTrack(audio, usbPath()));

    EXPECT_TRUE(QDir(engineLibraryPath()).exists());
    EXPECT_FALSE(QDir(usbPath() + QStringLiteral("/PIONEER")).exists())
            << "the old, misleading Pioneer output path must not come back";

    auto db = djinterop::engine::load_database(engineLibraryPath().toStdString());
    const auto tracks = db.tracks();
    ASSERT_EQ(tracks.size(), 1u);
    EXPECT_EQ(tracks[0].title().value_or(""), "One");

    // create_track() used to throw for every track because relative_path was
    // never set. It must now be set, and point at the audio copied next to
    // the database rather than at nothing.
    EXPECT_EQ(tracks[0].relative_path(), "../Mixxx Music/1 - one.mp3");
    EXPECT_TRUE(QFile::exists(usbPath() + QStringLiteral("/Mixxx Music/1 - one.mp3")));
}

// Export must not depend on the source audio still being readable: a missing
// file is logged and skipped, the track's metadata is still written.
TEST_F(EngineExporterTest, ExportTrackWithMissingAudioStillWritesMetadata) {
    ASSERT_TRUE(m_settingsDir.isValid());
    ASSERT_TRUE(m_usbDir.isValid());
    const QString ghost = QStringLiteral("C:/definitely/not/here/ghost.mp3");
    writeMixxxDb(settingsPath(), {{1, ghost, QStringLiteral("Ghost"), {}}}, {});

    EngineExporter exporter(settingsPath());
    ASSERT_TRUE(exporter.exportTrack(ghost, usbPath()));

    auto db = djinterop::engine::load_database(engineLibraryPath().toStdString());
    ASSERT_EQ(db.tracks().size(), 1u);
    EXPECT_FALSE(QFile::exists(usbPath() + QStringLiteral("/Mixxx Music/1 - ghost.mp3")));
}

// TODO 15: the engine database is opened once for the whole crate and every
// track lands in it. The old code called create_database() per track.
TEST_F(EngineExporterTest, ExportCrateExportsEveryTrackIntoOneDatabase) {
    ASSERT_TRUE(m_settingsDir.isValid());
    ASSERT_TRUE(m_usbDir.isValid());
    writeMixxxDb(settingsPath(),
            {{1, QStringLiteral("C:/music/a.mp3"), QStringLiteral("Alpha"), {}},
                    {2, QStringLiteral("C:/music/b.mp3"), QStringLiteral("Bravo"), {}},
                    {3, QStringLiteral("C:/music/c.mp3"), QStringLiteral("Charlie"), {}}},
            {{QStringLiteral("Set"), {1, 2, 3}}});

    EngineExporter exporter(settingsPath());
    QSignalSpy complete(&exporter, &EngineExporter::exportComplete);
    ASSERT_TRUE(exporter.exportCrate(QStringLiteral("Set"), usbPath()));

    auto db = djinterop::engine::load_database(engineLibraryPath().toStdString());
    EXPECT_EQ(db.tracks().size(), 3u);

    // exportComplete used to fire once per track PLUS once for the crate. It
    // now fires exactly once, with the real success count.
    ASSERT_EQ(complete.count(), 1);
    EXPECT_TRUE(complete.first().at(0).toBool());
    EXPECT_TRUE(complete.first().at(1).toString().contains(QStringLiteral("3/3")))
            << complete.first().at(1).toString().toStdString();
}

TEST_F(EngineExporterTest, ExportCrateReportsProgressForEachTrack) {
    ASSERT_TRUE(m_settingsDir.isValid());
    ASSERT_TRUE(m_usbDir.isValid());
    writeMixxxDb(settingsPath(),
            {{1, QStringLiteral("C:/music/a.mp3"), QStringLiteral("Alpha"), {}},
                    {2, QStringLiteral("C:/music/b.mp3"), QStringLiteral("Bravo"), {}}},
            {{QStringLiteral("Pair"), {1, 2}}});

    EngineExporter exporter(settingsPath());
    QSignalSpy progress(&exporter, &EngineExporter::exportProgress);
    ASSERT_TRUE(exporter.exportCrate(QStringLiteral("Pair"), usbPath()));

    ASSERT_EQ(progress.count(), 2);
    EXPECT_EQ(progress.at(0).at(0).toInt(), 1);
    EXPECT_EQ(progress.at(1).at(0).toInt(), 2);
    EXPECT_EQ(progress.at(1).at(1).toInt(), 2);
}

TEST_F(EngineExporterTest, ExportCrateUnknownCrateFailsAndCreatesNoOutput) {
    ASSERT_TRUE(m_settingsDir.isValid());
    ASSERT_TRUE(m_usbDir.isValid());
    writeMixxxDb(settingsPath(), {{1, QStringLiteral("C:/music/a.mp3"), QStringLiteral("Alpha"), {}}}, {});

    EngineExporter exporter(settingsPath());
    QSignalSpy complete(&exporter, &EngineExporter::exportComplete);
    EXPECT_FALSE(exporter.exportCrate(QStringLiteral("does not exist"), usbPath()));

    ASSERT_EQ(complete.count(), 1);
    EXPECT_FALSE(complete.first().at(0).toBool());
    EXPECT_TRUE(complete.first().at(1).toString().contains(QStringLiteral("not found")));
    EXPECT_FALSE(QDir(engineLibraryPath()).exists());
}

// The export root can't be created (here: it's an existing regular file, not a
// directory), so there is nowhere to put an Engine Library. That must fail
// loudly and once, not report a successful empty export.
TEST_F(EngineExporterTest, ExportCrateFailsWhenEngineDatabaseCannotBeCreated) {
    ASSERT_TRUE(m_settingsDir.isValid());
    const QString audio = makeAudio(QStringLiteral("a.mp3"));
    writeMixxxDb(settingsPath(), {{1, audio, QStringLiteral("Alpha"), {}}}, {{QStringLiteral("Set"), {1}}});
    const QString notADirectory = makeAudio(QStringLiteral("iam-a-file"));

    EngineExporter exporter(settingsPath());
    QSignalSpy complete(&exporter, &EngineExporter::exportComplete);
    EXPECT_FALSE(exporter.exportCrate(QStringLiteral("Set"), notADirectory));

    ASSERT_EQ(complete.count(), 1);
    EXPECT_FALSE(complete.first().at(0).toBool());
}

TEST_F(EngineExporterTest, ExportCrateEmptyCrateFails) {
    ASSERT_TRUE(m_settingsDir.isValid());
    ASSERT_TRUE(m_usbDir.isValid());
    writeMixxxDb(settingsPath(), {}, {{QStringLiteral("Empty"), {}}});

    EngineExporter exporter(settingsPath());
    QSignalSpy complete(&exporter, &EngineExporter::exportComplete);
    EXPECT_FALSE(exporter.exportCrate(QStringLiteral("Empty"), usbPath()));

    ASSERT_EQ(complete.count(), 1);
    EXPECT_TRUE(complete.first().at(1).toString().contains(QStringLiteral("empty")));
}

TEST_F(EngineExporterTest, ExportTrackNotInLibraryFailsAndSignalsFailure) {
    ASSERT_TRUE(m_settingsDir.isValid());
    ASSERT_TRUE(m_usbDir.isValid());
    writeMixxxDb(settingsPath(), {{1, QStringLiteral("C:/music/a.mp3"), QStringLiteral("Alpha"), {}}}, {});

    EngineExporter exporter(settingsPath());
    QSignalSpy complete(&exporter, &EngineExporter::exportComplete);
    EXPECT_FALSE(exporter.exportTrack(QStringLiteral("C:/music/not-in-library.mp3"), usbPath()));

    ASSERT_EQ(complete.count(), 1);
    EXPECT_FALSE(complete.first().at(0).toBool());
}

// TODO 14: only real hot cues are exported. The filter used to compare against
// the literal 2, which is MainCue - so every hot cue was dropped and the main
// cue exported in its place.
TEST_F(EngineExporterTest, HotCuesExportedAndMainCueIsNotMistakenForOne) {
    ASSERT_TRUE(m_settingsDir.isValid());
    ASSERT_TRUE(m_usbDir.isValid());
    writeMixxxDb(settingsPath(),
            {{1,
                    QStringLiteral("C:/music/cued.mp3"),
                    QStringLiteral("Cued"),
                    {{static_cast<int>(mixxx::CueType::HotCue), 0, 44100.0, QStringLiteral("Drop")},
                            {static_cast<int>(mixxx::CueType::MainCue), -1, 1000.0, QString()}}}},
            {});

    EngineExporter exporter(settingsPath());
    ASSERT_TRUE(exporter.exportTrack(QStringLiteral("C:/music/cued.mp3"), usbPath()));

    auto db = djinterop::engine::load_database(engineLibraryPath().toStdString());
    const auto tracks = db.tracks();
    ASSERT_EQ(tracks.size(), 1u);

    const auto hot = tracks[0].hot_cue_at(0);
    ASSERT_TRUE(hot.has_value()) << "hot cue 0 was dropped";
    EXPECT_EQ(hot->label, "Drop");
    EXPECT_DOUBLE_EQ(hot->sample_offset, 44100.0);
    EXPECT_FALSE(tracks[0].hot_cue_at(1).has_value())
            << "the main cue must not have been exported as a hot cue";
}
