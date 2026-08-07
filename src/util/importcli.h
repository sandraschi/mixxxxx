#pragma once

#include <QString>
#include <QStringList>

class TrackCollectionManager;

namespace mixxx {

class ImportCli {
  public:
    /// Parse track file paths from M3U/M3U8/PLS/CSV, Serato `.crate`, or VDJ `.vdjfolder`.
    /// No database side effects — used by CLI and unit tests.
    static QStringList parseImportLocations(
            const QString& sourcePath,
            QString* pError = nullptr);

    /// Import a playlist/crate file into a Mixxx crate, creating the crate if needed.
    /// Supports M3U/M3U8/PLS/CSV, VirtualDJ .vdjfolder, and Serato .crate track lists.
    static bool executeImportCrate(
            const QString& sourcePath,
            const QString& intoCrateName,
            TrackCollectionManager* pTrackCollectionManager,
            QString* pError = nullptr);
};

} // namespace mixxx
