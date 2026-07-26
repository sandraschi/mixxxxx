#include "dialog/dlgstartupbanner.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QSvgWidget>
#include <QVBoxLayout>

#include "moc_dlgstartupbanner.cpp"
#include "util/cmdlineargs.h"
#include "util/versionstore.h"

namespace mixxx {
namespace {

const ConfigKey kStartupBannerShowKey("[Config]", "startup_banner_show");

QString resolveBannerSvgPath() {
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
            QDir(appDir).filePath(QStringLiteral("../res/images/mixxxxx-banner.svg")),
            QDir(appDir).filePath(QStringLiteral("res/images/mixxxxx-banner.svg")),
            QDir(appDir).filePath(QStringLiteral("mixxxxx-banner.svg")),
    };
    for (const QString& candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return QFileInfo(candidate).absoluteFilePath();
        }
    }
    return QString();
}

QString featureHtml() {
    return QStringLiteral(
            "<p style='margin-top:0;'><b>Mixxxxx</b> is a video-enabled Mixxx fork for live "
            "AV mixing, streaming rigs, and fleet automation.</p>"
            "<ul style='margin-top:8px; line-height:1.45;'>"
            "<li><b>Video:</b> FFmpeg per deck, crossfader blend, beat FX (strobe &amp; zoom)</li>"
            "<li><b>Fallback chain:</b> pool loops → generative beats → Ken Burns album art</li>"
            "<li><b>Skins:</b> <i>Mixxxxx Video</i> — preview + output panels built in</li>"
            "<li><b>Export / import:</b> crates to Engine Prime, Serato, VirtualDJ</li>"
            "<li><b>OSC:</b> UDP 11119 in / 11118 out — mixx-dj-mcp companion</li>"
            "<li><b>NDI:</b> network video out when built with <code>-DNDI=ON</code> + SDK</li>"
            "<li><b>CLI:</b> <code>--gig-script</code>, <code>--set-control</code>, "
            "<code>--export-crate</code>, <code>--video-pool</code></li>"
            "</ul>"
            "<p style='color:#666; font-size:11px; margin-bottom:0;'>"
            "Version %1 · git %2 · %3</p>")
            .arg(VersionStore::version(),
                    VersionStore::gitVersion(),
                    VersionStore::platform());
}

} // namespace

void DlgStartupBanner::maybeShow(QWidget* parent,
        const UserSettingsPointer& pConfig,
        const CmdlineArgs& args) {
    if (args.getNoBanner() || args.getDumpControls()) {
        return;
    }
    if (!pConfig->getValue<bool>(kStartupBannerShowKey, true)) {
        return;
    }

    DlgStartupBanner dialog(parent, pConfig);
    dialog.exec();
}

DlgStartupBanner::DlgStartupBanner(QWidget* parent, const UserSettingsPointer& pConfig)
        : QDialog(parent),
          m_pConfig(pConfig) {
    setWindowTitle(tr("Welcome to Mixxxxx"));
    setModal(true);
    resize(560, 520);

    auto* pRootLayout = new QVBoxLayout(this);
    pRootLayout->setSpacing(12);
    pRootLayout->setContentsMargins(16, 16, 16, 12);

    const QString svgPath = resolveBannerSvgPath();
    if (!svgPath.isEmpty()) {
        auto* pBanner = new QSvgWidget(svgPath, this);
        pBanner->setMinimumHeight(110);
        pBanner->setMaximumHeight(140);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        pBanner->renderer()->setAspectRatioMode(Qt::KeepAspectRatio);
#endif
        pRootLayout->addWidget(pBanner);
    } else {
        auto* pTitle = new QLabel(
                QStringLiteral("<h2 style='margin:0; color:#ff8c00;'>Mixxxxx</h2>"), this);
        pTitle->setTextFormat(Qt::RichText);
        pRootLayout->addWidget(pTitle);
    }

    auto* pLine = new QFrame(this);
    pLine->setFrameShape(QFrame::HLine);
    pLine->setFrameShadow(QFrame::Sunken);
    pRootLayout->addWidget(pLine);

    auto* pBody = new QLabel(featureHtml(), this);
    pBody->setWordWrap(true);
    pBody->setTextFormat(Qt::RichText);
    pBody->setOpenExternalLinks(false);
    pRootLayout->addWidget(pBody, 1);

    auto* pShowAgain = new QCheckBox(tr("Show this welcome on startup"), this);
    pShowAgain->setChecked(true);
    pRootLayout->addWidget(pShowAgain);

    auto* pButtons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    pButtons->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    connect(pButtons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    pRootLayout->addWidget(pButtons);

    connect(pShowAgain, &QCheckBox::toggled, this, [this](bool checked) {
        m_pConfig->setValue(kStartupBannerShowKey, checked ? 1 : 0);
    });
}

void DlgStartupBanner::accept() {
    QDialog::accept();
}

} // namespace mixxx
