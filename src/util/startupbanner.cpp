#include "util/startupbanner.h"

#include <stdio.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include "util/cmdlineargs.h"
#include "util/versionstore.h"

namespace mixxx {
namespace {

void printColor(const CmdlineArgs& args, const char* code, const char* text) {
    if (args.useColors()) {
        fprintf(stderr, "%s%s\033[0m", code, text);
    } else {
        fputs(text, stderr);
    }
}

void println(const char* text) {
    fputs(text, stderr);
    fputc('\n', stderr);
}

QString bannerSvgPath() {
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
            QDir(appDir).filePath(QStringLiteral("../res/images/mixxxxx-banner.svg")),
            QDir(appDir).filePath(QStringLiteral("res/images/mixxxxx-banner.svg")),
            QDir(appDir).filePath(QStringLiteral("mixxxxx-banner.svg")),
    };
    for (const QString& candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return QDir::toNativeSeparators(QFileInfo(candidate).absoluteFilePath());
        }
    }
    return QStringLiteral("res/images/mixxxxx-banner.svg");
}

} // namespace

void StartupBanner::print(const CmdlineArgs& args) {
    const QString gitVersion = VersionStore::gitVersion();
    const QDateTime commitDate = VersionStore::date();
    const QString commitDateStr = commitDate.isValid()
            ? commitDate.toString(QStringLiteral("yyyy-MM-dd HH:mm"))
            : QStringLiteral("unknown");
    const QString platform = VersionStore::platform();

#ifndef DISABLE_BUILDTIME
    const QString builtOn = QStringLiteral("%1 @ %2").arg(__DATE__, __TIME__);
#else
    const QString builtOn = QStringLiteral("build time disabled");
#endif

    fputc('\n', stderr);
    printColor(args, "\033[38;5;214m", "  ╔══════════════════════════════════════════════════════════════╗\n");
    printColor(args, "\033[38;5;214m", "  ║  ");
    printColor(args, "\033[1;38;5;220m", "Mixxxxx");
    printColor(args, "\033[38;5;214m", "  ·  video-enabled Mixxx fork                              ║\n");
    printColor(args, "\033[38;5;214m", "  ╠══════════════════════════════════════════════════════════════╣\n");
    printColor(args, "\033[38;5;245m", "  ║  ▶ FFmpeg video per deck + crossfader blend                  ║\n");
    printColor(args, "\033[38;5;245m", "  ║  ▶ Rekordbox / Serato / VirtualDJ export                     ║\n");
    printColor(args, "\033[38;5;245m", "  ║  ▶ Phase indicator · ONNX stems (optional build)             ║\n");
    printColor(args, "\033[38;5;245m", "  ║  ▶ CLI: --export-crate · --import-crate · --gig-script         ║\n");
    printColor(args, "\033[38;5;245m", "  ║  ▶ Companion: mixx-dj-mcp (OSC on 11118/11119)                ║\n");
    printColor(args, "\033[38;5;214m", "  ╚══════════════════════════════════════════════════════════════╝\n");
    fputc('\n', stderr);

    const QString infoLine = QStringLiteral("  Mixxx %1 · git %2 · commit %3 · built %4 · %5")
                                     .arg(VersionStore::version(),
                                             gitVersion,
                                             commitDateStr,
                                             builtOn,
                                             platform);
    println(infoLine.toLocal8Bit().constData());

    const QString svgLine = QStringLiteral("  Banner SVG: %1").arg(bannerSvgPath());
    println(svgLine.toLocal8Bit().constData());
    println("  Tip: mixxx.exe --gig-script gigs\\your-set.mixxx");
    fputc('\n', stderr);
}

} // namespace mixxx
