#include "export/export_controls.h"
#include "export/rekordboxexporter.h"
#include "export/seratoexporter.h"
#include "export/virtualdjexporter.h"

#include <QStandardPaths>
#include <QDebug>

#include "control/controlobject.h"
#include "control/controlpushbutton.h"

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
            if (v <= 0) return; triggerRekordboxExport("[Channel1]", settingsPath); });
        connect(m_deck2.get(), &ControlPushButton::valueChanged, this, [this, settingsPath](double v) {
            if (v <= 0) return; triggerRekordboxExport("[Channel2]", settingsPath); });
        connect(m_deck3.get(), &ControlPushButton::valueChanged, this, [this, settingsPath](double v) {
            if (v <= 0) return; triggerRekordboxExport("[Channel3]", settingsPath); });
        connect(m_deck4.get(), &ControlPushButton::valueChanged, this, [this, settingsPath](double v) {
            if (v <= 0) return; triggerRekordboxExport("[Channel4]", settingsPath); });

        // ── Crate export ──
        m_exportCrate.reset(new ControlPushButton(ConfigKey("[Export]", "export_crate")));
        connect(m_exportCrate.get(), &ControlPushButton::valueChanged, this, [this, settingsPath](double v) {
            if (v <= 0) return;
            auto* ex = new RekordboxExporter(settingsPath, this);
            connect(ex, &RekordboxExporter::exportComplete, this, [ex](bool, const QString&) { ex->deleteLater(); });
            ex->exportCrate(QString(), QString());
        });

        // ── Serato export ──
        m_exportSerato.reset(new ControlPushButton(ConfigKey("[Export]", "export_serato")));
        connect(m_exportSerato.get(), &ControlPushButton::valueChanged, this, [this, settingsPath](double v) {
            if (v <= 0) return;
            auto* ex = new SeratoExporter(settingsPath, this);
            connect(ex, &SeratoExporter::exportComplete, this, [ex](bool, const QString&) { ex->deleteLater(); });
            ex->exportLibrary(QString());
        });

        // ── VirtualDJ export ──
        m_exportVdj.reset(new ControlPushButton(ConfigKey("[Export]", "export_virtualdj")));
        connect(m_exportVdj.get(), &ControlPushButton::valueChanged, this, [this, settingsPath](double v) {
            if (v <= 0) return;
            auto* ex = new VirtualDjExporter(settingsPath, this);
            connect(ex, &VirtualDjExporter::exportComplete, this, [ex](bool, const QString&) { ex->deleteLater(); });
            ex->exportLibrary(QString());
        });

        // ── Path COs ──
        m_rekordboxPath.reset(new ControlObject(ConfigKey("[Export]", "rekordbox_usb_path"), false, false, true, 0));
        m_seratoPath.reset(new ControlObject(ConfigKey("[Export]", "serato_path"), false, false, true, 0));
        m_vdjPath.reset(new ControlObject(ConfigKey("[Export]", "virtualdj_path"), false, false, true, 0));
    }

    void triggerRekordboxExport(const QString& group, const QString& settingsPath) {
        auto* ex = new RekordboxExporter(settingsPath, this);
        connect(ex, &RekordboxExporter::exportComplete, this, [ex](bool, const QString&) { ex->deleteLater(); });
        ex->exportTrack(group, QString());
    }

  private:
    std::unique_ptr<ControlPushButton> m_deck1, m_deck2, m_deck3, m_deck4;
    std::unique_ptr<ControlPushButton> m_exportCrate, m_exportSerato, m_exportVdj;
    std::unique_ptr<ControlObject> m_rekordboxPath, m_seratoPath, m_vdjPath;
};

void registerExportControls() {
    static ExportController controller;
}
