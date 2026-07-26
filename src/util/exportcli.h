#pragma once

#include <QString>

namespace mixxx {

enum class ExportFormat {
    Engine,
    Serato,
    VirtualDj,
};

class ExportCli {
  public:
    static bool parseExportFormat(
            const QString& format,
            ExportFormat* pFormat,
            QString* pError = nullptr);

    static bool executeExportCrate(
            const QString& crateName,
            ExportFormat format,
            const QString& exportPath,
            const QString& settingsPath,
            QString* pError = nullptr);
};

} // namespace mixxx
