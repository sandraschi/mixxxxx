#include "export/engineexporter.h"

#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSharedPointer>
#include <QtDebug>
#include <djinterop/engine/engine.hpp>

#include "library/export/engineprimeexportjob.h"
#include "library/export/engineprimeexportrequest.h"
#include "library/trackcollection.h"
#include "library/trackcollectionmanager.h"
#include "library/trackset/crate/crate.h"
#include "library/trackset/crate/cratestorage.h"
#include "track/track.h"
#include "video/companionvideo.h"

#include "moc_engineexporter.cpp"

namespace e = djinterop::engine;

namespace {

// Same directory upstream's export dialog uses for the audio copies, so a
// GUI export and a CLI export to the same stick share one music folder.
const QString kMusicDirName = QStringLiteral("mixxx-export");
const QString kVideoManifestName = QStringLiteral("mixxx-video.json");

// Audio copies are named "<trackId> - <original filename>" by upstream.
QString exportedName(const TrackRef& ref, const QString& fileName) {
    return ref.getId().toString() + QStringLiteral(" - ") + fileName;
}

void copyIfNewer(const QString& src, const QString& dst) {
    if (!QFile::exists(dst) ||
            QFileInfo(src).lastModified() > QFileInfo(dst).lastModified()) {
        if (QFile::exists(dst)) {
            QFile::remove(dst);
        }
        if (!QFile::copy(src, dst)) {
            qWarning() << "EngineExport: could not copy" << src << "to" << dst;
        }
    }
}

// Copies each candidate's companion video next to its exported audio and
// merges the result into the manifest. Returns the number of videos carried.
int exportCompanionVideos(const QList<TrackRef>& candidates, const QDir& musicDir) {
    QJsonObject byTrackId;
    const QString manifestPath = musicDir.filePath(kVideoManifestName);
    QFile manifestFile(manifestPath);
    if (manifestFile.open(QIODevice::ReadOnly)) {
        const QJsonDocument old = QJsonDocument::fromJson(manifestFile.readAll());
        for (const QJsonValue& v : old.object().value(QStringLiteral("tracks")).toArray()) {
            const QJsonObject o = v.toObject();
            byTrackId.insert(o.value(QStringLiteral("trackId")).toString(), o);
        }
        manifestFile.close();
    }

    int carried = 0;
    for (const TrackRef& ref : candidates) {
        const QString audioSrc = ref.getLocation();
        const QString videoSrc = mixxx::findCompanionVideoFor(audioSrc);
        if (videoSrc.isEmpty()) {
            continue;
        }
        const QFileInfo audioInfo(audioSrc);
        const QString audioName = exportedName(ref, audioInfo.fileName());
        // Skipped by upstream (unsupported type): no audio on the stick, so
        // a video for it would be an orphan.
        if (!QFile::exists(musicDir.filePath(audioName))) {
            continue;
        }
        const QString videoName = exportedName(ref, audioInfo.completeBaseName()) +
                QStringLiteral(".") + QFileInfo(videoSrc).suffix();
        copyIfNewer(videoSrc, musicDir.filePath(videoName));
        if (!QFile::exists(musicDir.filePath(videoName))) {
            continue;
        }
        byTrackId.insert(ref.getId().toString(),
                QJsonObject{
                        {QStringLiteral("trackId"), ref.getId().toString()},
                        {QStringLiteral("audio"), audioName},
                        {QStringLiteral("video"), videoName},
                });
        ++carried;
    }

    if (carried > 0) {
        QJsonArray tracks;
        for (auto it = byTrackId.begin(); it != byTrackId.end(); ++it) {
            tracks.append(it.value());
        }
        QFile out(manifestPath);
        if (out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            out.write(QJsonDocument(QJsonObject{
                                            {QStringLiteral("version"), 1},
                                            {QStringLiteral("tracks"), tracks},
                                    })
                              .toJson());
        } else {
            qWarning() << "EngineExport: could not write" << manifestPath;
        }
    }
    return carried;
}

} // namespace

EngineExporter::EngineExporter(TrackCollectionManager* pTrackCollectionManager, QObject* parent)
        : QObject(parent),
          m_pTrackCollectionManager(pTrackCollectionManager) {
}

QString EngineExporter::engineLibraryDirName() {
    return QString::fromStdString(std::string(e::default_database_dir_name));
}

QString EngineExporter::musicDirName() {
    return kMusicDirName;
}

QString EngineExporter::videoManifestFileName() {
    return kVideoManifestName;
}

void EngineExporter::fail(const QString& message) {
    qWarning() << "EngineExport:" << message;
    emit exportComplete(false, message);
}

bool EngineExporter::startCrate(const QString& crateName, const QString& usbPath) {
    Crate crate;
    if (crateName.trimmed().isEmpty() ||
            !m_pTrackCollectionManager->internalCollection()->crates().readCrateByName(
                    crateName, &crate)) {
        fail(QStringLiteral("Crate '%1' not found").arg(crateName));
        return false;
    }

    // Same track list upstream will export; needed here for the video pass.
    QList<TrackRef> candidates;
    auto& trackDao = m_pTrackCollectionManager->internalCollection()->getTrackDAO();
    auto result = m_pTrackCollectionManager->internalCollection()
                          ->crates()
                          .selectCrateTracksSorted(crate.getId());
    while (result.next()) {
        const TrackId id = result.trackId();
        candidates.append(TrackRef::fromFilePath(trackDao.getTrackLocation(id), id));
    }
    if (candidates.isEmpty()) {
        fail(QStringLiteral("Crate '%1' is empty").arg(crateName));
        return false;
    }
    return start(usbPath, {crate.getId()}, {}, candidates,
            QStringLiteral("crate '%1'").arg(crateName));
}

bool EngineExporter::startTrack(const TrackPointer& pTrack, const QString& usbPath) {
    if (!pTrack || !pTrack->getId().isValid()) {
        fail(QStringLiteral("Track is not in the library"));
        return false;
    }
    const TrackRef ref = TrackRef::fromFilePath(pTrack->getLocation(), pTrack->getId());
    return start(usbPath, {}, {ref}, {ref}, QStringLiteral("track"));
}

bool EngineExporter::start(const QString& usbPath,
        const QSet<CrateId>& crateIds,
        const QList<TrackRef>& explicitTracks,
        const QList<TrackRef>& videoCandidates,
        const QString& what) {
    const QDir root(usbPath);
    if (!root.exists() && !QDir().mkpath(root.absolutePath())) {
        fail(QStringLiteral("Cannot create export directory %1").arg(usbPath));
        return false;
    }

    auto pRequest = QSharedPointer<mixxx::EnginePrimeExportRequest>::create();
    pRequest->engineLibraryDbDir.setPath(root.filePath(engineLibraryDirName()));
    pRequest->musicFilesDir.setPath(root.filePath(kMusicDirName));
    pRequest->exportSchemaVersion = e::latest_schema;
    pRequest->crateIdsToExport = crateIds;
    pRequest->explicitTracks = explicitTracks;
    // Upstream requires this directory to exist before the job starts.
    QDir().mkpath(pRequest->engineLibraryDbDir.absolutePath());

    auto* pJob = new mixxx::EnginePrimeExportJob(this, m_pTrackCollectionManager, pRequest);
    connect(pJob,
            &mixxx::EnginePrimeExportJob::jobProgress,
            this,
            [this](int progress) { emit exportProgress(progress, 0); });
    connect(pJob,
            &mixxx::EnginePrimeExportJob::jobMaximum,
            this,
            [this](int maximum) { emit exportProgress(0, maximum); });
    connect(pJob,
            &mixxx::EnginePrimeExportJob::failed,
            this,
            [this](const QString& message) { fail(message); });
    const QString musicDir = pRequest->musicFilesDir.absolutePath();
    connect(pJob,
            &mixxx::EnginePrimeExportJob::completed,
            this,
            [this, videoCandidates, musicDir, what](int numTracks, int numCrates) {
                finishWithVideo(videoCandidates, musicDir, numTracks, numCrates, what);
            });
    connect(pJob, &QThread::finished, pJob, &QObject::deleteLater);
    pJob->start();
    return true;
}

void EngineExporter::finishWithVideo(const QList<TrackRef>& candidates,
        const QString& musicDir,
        int numTracks,
        int numCrates,
        const QString& what) {
    const int videos = exportCompanionVideos(candidates, QDir(musicDir));
    emit exportComplete(numTracks > 0,
            QStringLiteral("Exported %1 track(s), %2 crate(s), %3 companion video(s) from %4")
                    .arg(numTracks)
                    .arg(numCrates)
                    .arg(videos)
                    .arg(what));
}

bool EngineExporter::exportCrate(const QString& crateName, const QString& usbPath) {
    QEventLoop loop;
    bool ok = false;
    bool done = false;
    const auto conn = connect(this,
            &EngineExporter::exportComplete,
            &loop,
            [&](bool success, const QString&) {
                ok = success;
                done = true;
                loop.quit();
            });
    if (!startCrate(crateName, usbPath)) {
        disconnect(conn);
        return false;
    }
    if (!done) {
        loop.exec();
    }
    disconnect(conn);
    return ok;
}
