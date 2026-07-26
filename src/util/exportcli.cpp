#include "util/exportcli.h"

#include <QDir>
#include <QtDebug>

#include "export/rekordboxexporter.h"
#include "export/seratoexporter.h"
#include "export/virtualdjexporter.h"

namespace mixxx {

bool ExportCli::parseExportFormat(
        const QString& format,
        ExportFormat* pFormat,
        QString* pError) {
    const QString normalized = format.trimmed().toLower();
    if (normalized == QStringLiteral("engine") ||
            normalized == QStringLiteral("rekordbox")) {
        *pFormat = ExportFormat::Engine;
        return true;
    }
    if (normalized == QStringLiteral("serato")) {
        *pFormat = ExportFormat::Serato;
        return true;
    }
    if (normalized == QStringLiteral("virtualdj") ||
            normalized == QStringLiteral("vdj")) {
        *pFormat = ExportFormat::VirtualDj;
        return true;
    }
    if (pError) {
        *pError = QStringLiteral(
                "Unknown export format '%1'. Use engine, serato, or virtualdj.")
                          .arg(format);
    }
    return false;
}

bool ExportCli::executeExportCrate(
        const QString& crateName,
        ExportFormat format,
        const QString& exportPath,
        const QString& settingsPath,
        QString* pError) {
    if (crateName.trimmed().isEmpty()) {
        if (pError) {
            *pError = QStringLiteral("Crate name must not be empty.");
        }
        return false;
    }
    if (exportPath.trimmed().isEmpty()) {
        if (pError) {
            *pError = QStringLiteral("Export path must not be empty.");
        }
        return false;
    }

    QDir exportDir(exportPath);
    if (!exportDir.exists() && !exportDir.mkpath(QStringLiteral("."))) {
        if (pError) {
            *pError = QStringLiteral("Cannot create export directory: %1")
                              .arg(exportPath);
        }
        return false;
    }

    const QString normalizedExportPath =
            QDir::toNativeSeparators(exportDir.absolutePath());
    const QString normalizedSettingsPath = settingsPath.endsWith('/')
            ? settingsPath
            : settingsPath + QStringLiteral("/");

    bool success = false;
    switch (format) {
    case ExportFormat::Engine: {
        RekordboxExporter exporter(normalizedSettingsPath);
        success = exporter.exportCrate(crateName, normalizedExportPath);
        break;
    }
    case ExportFormat::Serato: {
        SeratoExporter exporter(normalizedSettingsPath);
        success = exporter.exportCrate(crateName, normalizedExportPath);
        break;
    }
    case ExportFormat::VirtualDj: {
        VirtualDjExporter exporter(normalizedSettingsPath);
        success = exporter.exportCrate(crateName, normalizedExportPath);
        break;
    }
    }

    if (!success && pError) {
        *pError = QStringLiteral("Export failed for crate '%1'.").arg(crateName);
    }
    return success;
}

} // namespace mixxx
