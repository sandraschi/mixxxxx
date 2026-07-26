#pragma once

#include <QString>

class TrackCollectionManager;

namespace mixxx {

class ImportCli {
  public:
    /// Import a playlist/crate file into a Mixxx crate, creating the crate if needed.
    /// Supports M3U/M3U8/PLS/CSV, VirtualDJ .vdjfolder, and Serato .crate track lists.
    static bool executeImportCrate(
            const QString& sourcePath,
            const QString& intoCrateName,
            TrackCollectionManager* pTrackCollectionManager,
            QString* pError = nullptr);
};

} // namespace mixxx
