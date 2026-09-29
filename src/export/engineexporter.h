#pragma once
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>

#include "library/trackset/crate/crateid.h"
#include "track/track_decl.h"
#include "track/trackref.h"

class TrackCollectionManager;


// Engine DJ export: delegates all library data (metadata, beatgrids,
// waveforms, hot cues, crates, cover art, audio copy) to upstream Mixxx's
// EnginePrimeExportJob, and adds what upstream does not know about - the
// companion video of each track. Companion videos are copied next to the
// exported audio using the same "<id> - <name>" naming (so mixxxxx finds
// them on the USB stick too) and listed in mixxx-video.json.
//
// Async by design: the job pulls data from the main thread, so callers must
// keep the main event loop running. Use exportCrate() for a blocking call
// from the CLI.
class EngineExporter : public QObject {
    Q_OBJECT
  public:
    explicit EngineExporter(TrackCollectionManager* pTrackCollectionManager,
            QObject* parent = nullptr);

    // Returns false (after emitting exportComplete(false, ...)) when the
    // export could not be started; otherwise exportComplete fires later.
    bool startCrate(const QString& crateName, const QString& usbPath);
    bool startTrack(const TrackPointer& pTrack, const QString& usbPath);

    // Blocking convenience: runs a local event loop until the export ends.
    bool exportCrate(const QString& crateName, const QString& usbPath);

    static QString engineLibraryDirName();
    static QString musicDirName();
    static QString videoManifestFileName();

  signals:
    void exportProgress(int current, int total);
    void exportComplete(bool success, const QString& message);

  private:
    bool start(const QString& usbPath,
            const QSet<CrateId>& crateIds,
            const QList<TrackRef>& explicitTracks,
            const QList<TrackRef>& videoCandidates,
            const QString& what);
    void finishWithVideo(const QList<TrackRef>& candidates,
            const QString& musicDir,
            int numTracks,
            int numCrates,
            const QString& what);
    void fail(const QString& message);

    TrackCollectionManager* m_pTrackCollectionManager;
};
