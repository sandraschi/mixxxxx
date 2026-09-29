#pragma once

#include <QFileInfo>
#include <QString>
#include <QStringList>

namespace mixxx {

// A track's companion video is a file next to the audio with the same base
// name. Shared by VideoWidget (playback) and EngineExporter (so exported
// tracks keep their video).
inline const QStringList& companionVideoExtensions() {
    static const QStringList kExtensions = {
            QStringLiteral(".mp4"),
            QStringLiteral(".mkv"),
            QStringLiteral(".mov"),
            QStringLiteral(".webm")};
    return kExtensions;
}

inline QString findCompanionVideoFor(const QString& audioPath) {
    const QFileInfo fi(audioPath);
    const QString basePath = fi.absolutePath() + QLatin1Char('/') + fi.completeBaseName();
    for (const QString& ext : companionVideoExtensions()) {
        const QString videoPath = basePath + ext;
        if (QFileInfo::exists(videoPath)) {
            return videoPath;
        }
    }
    return QString();
}

} // namespace mixxx
