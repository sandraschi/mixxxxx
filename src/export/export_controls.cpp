#include "export/export_controls.h"
#include "export/rekordboxexporter.h"
#include "export/seratoexporter.h"
#include "export/virtualdjexporter.h"

#include <QStandardPaths>
#include <QDebug>
#include <QDir>

#include "control/controlobject.h"
#include "control/controlpushbutton.h"
#include "mixer/basetrackplayer.h"
#include "mixer/playermanager.h"
#include "track/track.h"

// Resolve the export path for GUI/OSC/MIDI-triggered exports (the trigger
// COs below carry no path argument, so there's nowhere to take an explicit
// one from - CLI exports are unaffected, see --export-path/cmdlineargs.cpp,
// which already requires an explicit path and never calls this function).
//
// This used to scan every mounted volume for "removable-looking" media -
// not NTFS, not the root drive, under 128GB - and write into the first
// match with zero confirmation (docs/TODO.md item 16). That heuristic could
// select a mounted ISO, an unrelated USB drive plugged in for something
// else, or a FAT32 EFI system partition: none of those are excluded by
// "not NTFS and under 128GB", and QStorageInfo::isRoot() only excludes the
// Windows system drive, not other mounted system volumes. Until this has a
// real path-picker UI, always using the same safe, already-implemented
// fallback is the only option that can't silently pick the wrong disk.
static QString defaultExportPath(const QString& subdir) {
    QString fallback = QStandardPaths::writableLocation(QStandardPaths::MusicLocation)
        + "/MixxxExports/" + subdir;
    QDir().mkpath(fallback);
    return fallback;
}

class ExportController : public QObject {
  public:
    explicit ExportController(PlayerManager* pPlayerManager)
            : m_pPlayerManager(pPlayerManager) {
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
        connect(m_exportCrate.get(), &ControlPushButton::valueChanged, this, [](double v) {
            if (v <= 0) return;
            // No crate selection mechanism exists yet. ControlObjects hold
            // doubles only, so the [Export],rekordbox_usb_path string CO in the
            // README is not implementable as documented. Calling exportCrate("")
            // would query for a crate named "" and always fail, so refuse
            // loudly instead of pretending. See docs/TODO.md item 12.
            qWarning() << "Export: no crate configured; crate export is not "
                          "implemented yet (docs/TODO.md item 12)";
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
        // The previous code passed `group` straight through as the trackPath
        // argument, so RekordboxExporter queried
        // `WHERE track_locations.location = '[Channel1]'` and per-deck export
        // failed 100% of the time. Resolve the loaded track first.
        if (!m_pPlayerManager) {
            qWarning() << "Export: no PlayerManager, cannot resolve deck" << group;
            return;
        }
        BaseTrackPlayer* pPlayer = m_pPlayerManager->getPlayer(group);
        if (!pPlayer) {
            qWarning() << "Export: no player for group" << group;
            return;
        }
        TrackPointer pTrack = pPlayer->getLoadedTrack();
        if (!pTrack) {
            qWarning() << "Export: no track loaded on" << group;
            return;
        }
        const QString trackPath = pTrack->getLocation();
        if (trackPath.isEmpty()) {
            qWarning() << "Export: loaded track on" << group << "has no location";
            return;
        }

        auto* ex = new RekordboxExporter(settingsPath, this);
        connect(ex, &RekordboxExporter::exportComplete, this, [ex](bool, const QString&) { ex->deleteLater(); });
        ex->exportTrack(trackPath, usbPath);
    }

  private:
    PlayerManager* m_pPlayerManager = nullptr;
    std::unique_ptr<ControlPushButton> m_deck1, m_deck2, m_deck3, m_deck4;
    std::unique_ptr<ControlPushButton> m_exportCrate, m_exportSerato, m_exportVdj;
};

void registerExportControls(PlayerManager* pPlayerManager) {
    static ExportController controller(pPlayerManager);
}
