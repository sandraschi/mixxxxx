#pragma once
#include <QObject>
#include <QString>

namespace djinterop {
class database;
}

// Writes Denon Engine DJ library format (via libdjinterop), NOT Pioneer's
// native DeviceSQL/.pdb format despite the class's old name (RekordboxExporter).
// See docs/TODO.md item 13.
class EngineExporter : public QObject {
    Q_OBJECT
  public:
    explicit EngineExporter(const QString& settingsPath, QObject* parent = nullptr);

    bool exportTrack(const QString& trackPath, const QString& usbPath);
    bool exportCrate(const QString& crateName, const QString& usbPath);

  signals:
    void exportProgress(int current, int total);
    void exportComplete(bool success, const QString& message);

  private:
    // Queries the Mixxx library DB for one track and writes it into an
    // already-open engine database. Split out of exportTrack() so
    // exportCrate() can open the engine database ONCE and reuse it across
    // every track in the crate, instead of calling create_database() per
    // track (docs/TODO.md item 15) - besides being wasteful, repeatedly
    // creating a database at a path that already has one is not a
    // supported/tested libdjinterop usage pattern.
    //
    // usbPath is the export root: the audio file is copied into a sibling
    // "Mixxx Music" folder there (same convention as upstream Mixxx's own
    // Engine Prime exporter, src/library/export/engineprimeexportjob.cpp)
    // and the track's relative_path - which libdjinterop REQUIRES and this
    // exporter never used to set, so create_track() threw for every track -
    // points at that copy relative to the Engine Library directory.
    bool exportTrackToDatabase(
            const QString& trackPath,
            const QString& usbPath,
            djinterop::database& engDb);

    QString m_settingsPath;
};
