#include "export/export_controls.h"
#include "export/rekordboxexporter.h"
#include "export/seratoexporter.h"
#include "export/virtualdjexporter.h"

#include <QStandardPaths>
#include <QDebug>
#include <QDir>
#include <QStorageInfo>

#include "control/controlobject.h"
#include "control/controlpushbutton.h"

// Resolve the best export path per exporter type
static QString defaultExportPath(const QString& subdir) {
    // Prefer removable drives (USB sticks) for physical export
    for (const QStorageInfo& storage : QStorageInfo::mountedVolumes()) {
        if (storage.isValid() && !storage.isRoot() &&
            storage.isReady() && storage.fileSystemType() != "NTFS" &&
            storage.bytesTotal() < 128LL * 1024 * 1024 * 1024) {
            QString root = storage.rootPath();
            QDir(root).mkpath(subdir);
            return root + subdir;
        }
    }
    // Fallback: user's Music folder
    QString fallback = QStandardPaths::writableLocation(QStandardPaths::MusicLocation)
        + "/MixxxExports/" + subdir;
    QDir().mkpath(fallback);
    return fallback;
}

class ExportController : public QObject {
  public:
    ExportController() {
        QString settingsPath = QStandardPaths::writableLocation(
            QStandardPaths::AppLocalDataLocation);

        // ── Deck export triggers ──
        m_deck1.reset(new ControlPushButton(ConfigKey("[Channel1]", "export_rekordbox")));
        m_deck2.reset(new ControlPushButton(ConfigKey("[Channel2]", "export_rekordbox")));
        m_deck3.reset(new ControlPushButton(ConfigKey("[Channel3]", "export_rekordbox")));
        m_deck4.reset(new ControlPushButton(ConfigKey("[Channel4]", "export_rekordbox")));

        connect(m_deck1.get(), &ControlPushButton::valueChanged, this, [this, settingsPath](double v) {
            if (v <= 0) return; triggerRekordboxExport("[Channel1]", settingsPath, defaultExportPath("PIONEER")); });
        connect(m_deck2.get(), &ControlPushButton::valueChanged, this, [this, settingsPath](double v) {
            if (v <= 0) return; triggerRekordboxExport("[Channel2]", settingsPath, defaultExportPath("PIONEER")); });
        connect(m_deck3.get(), &ControlPushButton::valueChanged, this, [this, settingsPath](double v) {
            if (v <= 0) return; triggerRekordboxExport("[Channel3]", settingsPath, defaultExportPath("PIONEER")); });
        connect(m_deck4.get(), &ControlPushButton::valueChanged, this, [this, settingsPath](double v) {
            if (v <= 0) return; triggerRekordboxExport("[Channel4]", settingsPath, defaultExportPath("PIONEER")); });

        // ── Crate export ──
        m_exportCrate.reset(new ControlPushButton(ConfigKey("[Export]", "export_crate")));
        connect(m_exportCrate.get(), &ControlPushButton::valueChanged, this, [this, settingsPath](double v) {
            if (v <= 0) return;
            auto* ex = new RekordboxExporter(settingsPath, this);
            connect(ex, &RekordboxExporter::exportComplete, this, [ex](bool, const QString&) { ex->deleteLater(); });
            ex->exportCrate(QString(), defaultExportPath("PIONEER"));
        });

        // ── Serato export ──
        m_exportSerato.reset(new ControlPushButton(ConfigKey("[Export]", "export_serato")));
        connect(m_exportSerato.get(), &ControlPushButton::valueChanged, this, [this, settingsPath](double v) {
            if (v <= 0) return;
            auto* ex = new SeratoExporter(settingsPath, this);
            connect(ex, &SeratoExporter::exportComplete, this, [ex](bool, const QString&) { ex->deleteLater(); });
            ex->exportLibrary(defaultExportPath("_Serato_"));
        });

        // ── VirtualDJ export ──
        m_exportVdj.reset(new ControlPushButton(ConfigKey("[Export]", "export_virtualdj")));
        connect(m_exportVdj.get(), &ControlPushButton::valueChanged, this, [this, settingsPath](double v) {
            if (v <= 0) return;
            auto* ex = new VirtualDjExporter(settingsPath, this);
            connect(ex, &VirtualDjExporter::exportComplete, this, [ex](bool, const QString&) { ex->deleteLater(); });
            ex->exportLibrary(defaultExportPath("_VirtualDJ"));
        });
    }

    void triggerRekordboxExport(const QString& group, const QString& settingsPath, const QString& usbPath) {
        auto* ex = new RekordboxExporter(settingsPath, this);
        connect(ex, &RekordboxExporter::exportComplete, this, [ex](bool, const QString&) { ex->deleteLater(); });
        ex->exportTrack(group, usbPath);
    }

  private:
    std::unique_ptr<ControlPushButton> m_deck1, m_deck2, m_deck3, m_deck4;
    std::unique_ptr<ControlPushButton> m_exportCrate, m_exportSerato, m_exportVdj;
};

void registerExportControls() {
    static ExportController controller;
}
