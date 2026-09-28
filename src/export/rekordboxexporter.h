#pragma once
#include <QObject>
#include <QString>

namespace djinterop {
class database;
}

class RekordboxExporter : public QObject {
    Q_OBJECT
  public:
    explicit RekordboxExporter(const QString& settingsPath, QObject* parent = nullptr);

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
    bool exportTrackToDatabase(const QString& trackPath, djinterop::database& engDb);

    QString m_settingsPath;
};
