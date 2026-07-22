#include "export/export_controls.h"

#include <memory>

#include "control/controlobject.h"
#include "control/controlpushbutton.h"

void registerExportControls() {
    static const auto usbPath = std::make_unique<ControlObject>(
            ConfigKey("[Export]", "rekordbox_usb_path"), false, false, true, 0);

    static const auto exportCrate = std::make_unique<ControlPushButton>(
            ConfigKey("[Export]", "export_crate"));

    static const auto exportDeck1 = std::make_unique<ControlPushButton>(
            ConfigKey("[Channel1]", "export_rekordbox"));

    static const auto exportDeck2 = std::make_unique<ControlPushButton>(
            ConfigKey("[Channel2]", "export_rekordbox"));

    static const auto exportDeck3 = std::make_unique<ControlPushButton>(
            ConfigKey("[Channel3]", "export_rekordbox"));

    static const auto exportDeck4 = std::make_unique<ControlPushButton>(
            ConfigKey("[Channel4]", "export_rekordbox"));

    static const auto exportSerato = std::make_unique<ControlPushButton>(
            ConfigKey("[Export]", "export_serato"));

    static const auto seratoPath = std::make_unique<ControlObject>(
            ConfigKey("[Export]", "serato_path"), false, false, true, 0);

    static const auto exportVirtualDj = std::make_unique<ControlPushButton>(
            ConfigKey("[Export]", "export_virtualdj"));

    static const auto virtualDjPath = std::make_unique<ControlObject>(
            ConfigKey("[Export]", "virtualdj_path"), false, false, true, 0);
}
