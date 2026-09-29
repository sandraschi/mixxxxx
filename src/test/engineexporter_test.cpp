#include <gtest/gtest.h>

#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <djinterop/djinterop.hpp>
#include <djinterop/engine/engine.hpp>

#include "export/engineexporter.h"
#include "library/trackcollection.h"
#include "library/trackset/crate/crate.h"
#include "test/librarytest.h"
#include "track/track.h"
#include "video/companionvideo.h"

namespace {

// Exports go through upstream's EnginePrimeExportJob (real library, real
// audio, real waveform/beatgrid code paths); these tests cover the wiring
// and the video layer mixxxxx adds on top.
class EngineExporterTest : public LibraryTest {
  protected:
    void SetUp() override {
        ASSERT_TRUE(m_srcDir.isValid());
        ASSERT_TRUE(m_outDir.isValid());
    }

    QString copyAudio(const QString& name) {
        const QString dst = m_srcDir.filePath(name);
        EXPECT_TRUE(QFile::copy(getTestDir().filePath(QStringLiteral("id3-test-data/artist.mp3")), dst));
        QFile::setPermissions(dst, QFile::ReadOwner | QFile::WriteOwner);
        return dst;
    }

    void addCompanionVideo(const QString& audioPath, const QString& ext) {
        QFile f(QFileInfo(audioPath).absolutePath() + "/" +
                QFileInfo(audioPath).completeBaseName() + ext);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("not really a video");
    }

    TrackPointer addTrack(const QString& path) {
        TrackPointer pTrack = getOrAddTrackByLocation(path);
        EXPECT_TRUE(pTrack);
        EXPECT_TRUE(pTrack->getId().isValid());
        return pTrack;
    }

    void makeCrate(const QString& name, const QList<TrackPointer>& tracks) {
        Crate crate;
        crate.setName(name);
        CrateId crateId;
        ASSERT_TRUE(internalCollection()->insertCrate(crate, &crateId));
        QList<TrackId> ids;
        for (const auto& t : tracks) {
            ids.append(t->getId());
        }
        if (!ids.isEmpty()) {
            ASSERT_TRUE(internalCollection()->addCrateTracks(crateId, ids));
        }
    }

    // Async start + local event loop, exactly what the CLI does.
    bool exportCrate(const QString& name) {
        EngineExporter exporter(trackCollectionManager());
        return exporter.exportCrate(name, out());
    }

    bool exportTrack(const TrackPointer& pTrack) {
        EngineExporter exporter(trackCollectionManager());
        QEventLoop loop;
        bool ok = false;
        QObject::connect(&exporter,
                &EngineExporter::exportComplete,
                &loop,
                [&](bool success, const QString&) {
                    ok = success;
                    loop.quit();
                });
        if (!exporter.startTrack(pTrack, out())) {
            return false;
        }
        loop.exec();
        return ok;
    }

    QString out() const {
        return m_outDir.path();
    }
    QDir musicDir() const {
        return QDir(out() + "/" + EngineExporter::musicDirName());
    }
    QString engineLibraryPath() const {
        return out() + "/" + EngineExporter::engineLibraryDirName();
    }
    QJsonArray manifestTracks() const {
        QFile f(musicDir().filePath(EngineExporter::videoManifestFileName()));
        if (!f.open(QIODevice::ReadOnly)) {
            return {};
        }
        return QJsonDocument::fromJson(f.readAll()).object().value("tracks").toArray();
    }

    QTemporaryDir m_srcDir;
    QTemporaryDir m_outDir;
};

TEST_F(EngineExporterTest, CrateExportWritesEngineLibraryAndCopiesAudio) {
    auto pTrack = addTrack(copyAudio("one.mp3"));
    makeCrate("Set", {pTrack});

    ASSERT_TRUE(exportCrate("Set"));

    auto db = djinterop::engine::load_database(engineLibraryPath().toStdString());
    ASSERT_EQ(db.tracks().size(), 1u);
    EXPECT_TRUE(QFile::exists(
            musicDir().filePath(pTrack->getId().toString() + " - one.mp3")));
    EXPECT_FALSE(QDir(out() + "/PIONEER").exists());
}

TEST_F(EngineExporterTest, CompanionVideoIsCarriedNextToExportedAudio) {
    const QString audio = copyAudio("clip.mp3");
    addCompanionVideo(audio, ".mp4");
    auto pTrack = addTrack(audio);
    makeCrate("Set", {pTrack});

    ASSERT_TRUE(exportCrate("Set"));

    const QString id = pTrack->getId().toString();
    EXPECT_TRUE(QFile::exists(musicDir().filePath(id + " - clip.mp3")));
    EXPECT_TRUE(QFile::exists(musicDir().filePath(id + " - clip.mp4")));
    // The exported pair follows the same companion convention, so mixxxxx
    // finds the video when the stick is used as a library source.
    EXPECT_EQ(mixxx::findCompanionVideoFor(musicDir().filePath(id + " - clip.mp3")),
            musicDir().filePath(id + " - clip.mp4"));

    const QJsonArray tracks = manifestTracks();
    ASSERT_EQ(tracks.size(), 1);
    EXPECT_EQ(tracks[0].toObject().value("trackId").toString(), id);
    EXPECT_EQ(tracks[0].toObject().value("audio").toString(), id + " - clip.mp3");
    EXPECT_EQ(tracks[0].toObject().value("video").toString(), id + " - clip.mp4");
}

TEST_F(EngineExporterTest, TrackWithoutVideoWritesNoManifest) {
    auto pTrack = addTrack(copyAudio("plain.mp3"));
    makeCrate("Set", {pTrack});

    ASSERT_TRUE(exportCrate("Set"));

    EXPECT_FALSE(QFile::exists(
            musicDir().filePath(EngineExporter::videoManifestFileName())));
}

TEST_F(EngineExporterTest, OnlyTracksWithVideoAreListedAndManifestMergesAcrossExports) {
    const QString withVideo = copyAudio("has.mp3");
    addCompanionVideo(withVideo, ".webm");
    auto pA = addTrack(withVideo);
    auto pB = addTrack(copyAudio("none.mp3"));
    makeCrate("First", {pA, pB});
    ASSERT_TRUE(exportCrate("First"));
    ASSERT_EQ(manifestTracks().size(), 1);

    const QString second = copyAudio("later.mp3");
    addCompanionVideo(second, ".mkv");
    auto pC = addTrack(second);
    makeCrate("Second", {pC});
    ASSERT_TRUE(exportCrate("Second"));

    EXPECT_EQ(manifestTracks().size(), 2) << "earlier entries must survive a later export";
}

TEST_F(EngineExporterTest, PerTrackExportExportsExactlyThatTrack) {
    const QString audio = copyAudio("deck.mp3");
    addCompanionVideo(audio, ".mov");
    auto pDeck = addTrack(audio);
    auto pOther = addTrack(copyAudio("other.mp3"));
    makeCrate("Unrelated", {pOther});

    ASSERT_TRUE(exportTrack(pDeck));

    auto db = djinterop::engine::load_database(engineLibraryPath().toStdString());
    EXPECT_EQ(db.tracks().size(), 1u);
    EXPECT_TRUE(QFile::exists(
            musicDir().filePath(pDeck->getId().toString() + " - deck.mp3")));
    EXPECT_TRUE(QFile::exists(
            musicDir().filePath(pDeck->getId().toString() + " - deck.mov")));
    EXPECT_FALSE(QFile::exists(
            musicDir().filePath(pOther->getId().toString() + " - other.mp3")));
}

TEST_F(EngineExporterTest, UnknownCrateFailsAndCreatesNoLibrary) {
    EXPECT_FALSE(exportCrate("nope"));
    EXPECT_FALSE(QDir(engineLibraryPath()).exists());
}

TEST_F(EngineExporterTest, EmptyCrateFails) {
    makeCrate("Empty", {});
    EXPECT_FALSE(exportCrate("Empty"));
}

TEST_F(EngineExporterTest, TrackNotInLibraryFails) {
    EngineExporter exporter(trackCollectionManager());
    EXPECT_FALSE(exporter.startTrack(TrackPointer(), out()));
}

} // namespace
