#pragma once

#include <QDialog>

#include "preferences/usersettings.h"

class CmdlineArgs;

namespace mixxx {

/// First-run / welcome dialog extolling Mixxxxx features.
/// Respects `[Config],startup_banner_show` and `--no-banner`.
class DlgStartupBanner : public QDialog {
    Q_OBJECT

  public:
    static void maybeShow(QWidget* parent,
            const UserSettingsPointer& pConfig,
            const CmdlineArgs& args);

  private:
    explicit DlgStartupBanner(QWidget* parent, const UserSettingsPointer& pConfig);

    void accept() override;

    UserSettingsPointer m_pConfig;
};

} // namespace mixxx
