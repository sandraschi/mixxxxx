#include "video/videopool.h"

#include "util/cmdlineargs.h"

#include <limits>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace {

const QStringList kVideoExtensions = {
        QStringLiteral(".mp4"),
        QStringLiteral(".mkv"),
        QStringLiteral(".mov"),
        QStringLiteral(".webm"),
};

bool hasVideoExtension(const QString& filePath) {
    const QString lower = filePath.toLower();
    for (const QString& ext : kVideoExtensions) {
        if (lower.endsWith(ext)) {
            return true;
        }
    }
    return false;
}

} // namespace

VideoPool& VideoPool::instance() {
    static VideoPool pool;
    return pool;
}

void VideoPool::setDirectoryOverride(const QString& directory) {
    m_directoryOverride = directory;
    m_loaded = false;
    m_entries.clear();
}

QString VideoPool::directory() const {
    if (!m_directoryOverride.isEmpty()) {
        return m_directoryOverride;
    }
    const QString fromCli = CmdlineArgs::Instance().getVideoPoolPath();
    if (!fromCli.isEmpty()) {
        return fromCli;
    }
    return QDir(CmdlineArgs::Instance().getSettingsPath()).filePath(QStringLiteral("video-pool"));
}

void VideoPool::reload() {
    m_entries.clear();
    m_loaded = true;

    const QDir poolDir(directory());
    if (!poolDir.exists()) {
        return;
    }

    const QFileInfoList files = poolDir.entryInfoList(QDir::Files | QDir::Readable);
    for (const QFileInfo& fi : files) {
        if (!hasVideoExtension(fi.absoluteFilePath())) {
            continue;
        }
        const VideoPoolEntry entry = entryFromFile(fi.absoluteFilePath());
        if (entry.bpm > 0.0) {
            m_entries.append(entry);
        }
    }
}

QVector<VideoPoolEntry> VideoPool::entries() const {
    ensureLoaded();
    return m_entries;
}

void VideoPool::ensureLoaded() const {
    if (m_loaded) {
        return;
    }
    const_cast<VideoPool*>(this)->reload();
}

double VideoPool::energyLabelToValue(const QString& label) {
    const QString normalized = label.trimmed().toLower();
    if (normalized == QStringLiteral("low")) {
        return 0.2;
    }
    if (normalized == QStringLiteral("medium") || normalized == QStringLiteral("med")) {
        return 0.5;
    }
    if (normalized == QStringLiteral("high")) {
        return 0.85;
    }
    bool ok = false;
    const double numeric = normalized.toDouble(&ok);
    if (ok) {
        return qBound(0.0, numeric, 1.0);
    }
    return 0.5;
}

std::optional<VideoPoolEntry> VideoPool::parseFileName(const QString& filePath) {
    const QFileInfo fi(filePath);
    static const QRegularExpression kPattern(
            QStringLiteral("^(\\d+(?:\\.\\d+)?)-([^-]+)-(.+)$"));
    const QRegularExpressionMatch match = kPattern.match(fi.completeBaseName());
    if (!match.hasMatch()) {
        return std::nullopt;
    }

    VideoPoolEntry entry;
    entry.path = fi.absoluteFilePath();
    entry.bpm = match.captured(1).toDouble();
    entry.genre = match.captured(2).trimmed();
    entry.energy = energyLabelToValue(match.captured(3));
    if (entry.bpm <= 0.0) {
        return std::nullopt;
    }
    return entry;
}

std::optional<VideoPoolEntry> VideoPool::parseSidecarJson(const QString& jsonPath) {
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return std::nullopt;
    }

    const QJsonObject obj = doc.object();
    const QFileInfo jsonInfo(jsonPath);
    const QDir jsonDir = jsonInfo.dir();

    VideoPoolEntry entry;
    const QString fileField = obj.value(QStringLiteral("file")).toString().trimmed();
    if (!fileField.isEmpty()) {
        entry.path = jsonDir.filePath(fileField);
    } else {
        QString videoName = jsonInfo.fileName();
        if (videoName.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive)) {
            videoName.chop(5);
        }
        entry.path = jsonDir.filePath(videoName);
        if (!hasVideoExtension(entry.path)) {
            entry.path += QStringLiteral(".mp4");
        }
    }

    entry.bpm = obj.value(QStringLiteral("bpm")).toDouble();
    entry.genre = obj.value(QStringLiteral("genre")).toString().trimmed();
    if (obj.contains(QStringLiteral("energy"))) {
        const QJsonValue energyValue = obj.value(QStringLiteral("energy"));
        if (energyValue.isDouble()) {
            entry.energy = qBound(0.0, energyValue.toDouble(), 1.0);
        } else {
            entry.energy = energyLabelToValue(energyValue.toString());
        }
    }

    if (entry.bpm <= 0.0 || !QFileInfo::exists(entry.path)) {
        return std::nullopt;
    }
    return entry;
}

VideoPoolEntry VideoPool::entryFromFile(const QString& filePath) const {
    const QString sidecarPath = filePath + QStringLiteral(".json");
    if (QFileInfo::exists(sidecarPath)) {
        if (const auto fromSidecar = parseSidecarJson(sidecarPath)) {
            return *fromSidecar;
        }
    }
    if (const auto fromName = parseFileName(filePath)) {
        return *fromName;
    }
    return VideoPoolEntry{};
}

double VideoPool::scoreEntry(const VideoPoolEntry& entry,
        double trackBpm,
        const QString& trackGenre,
        double trackEnergy) const {
    const double safeTrackBpm = trackBpm > 0.0 ? trackBpm : entry.bpm;
    const double bpmDiff = qAbs(entry.bpm - safeTrackBpm) / safeTrackBpm;

    double genrePenalty = 1.0;
    if (!trackGenre.isEmpty() && !entry.genre.isEmpty()) {
        if (entry.genre.compare(trackGenre, Qt::CaseInsensitive) == 0) {
            genrePenalty = 0.0;
        } else if (trackGenre.contains(entry.genre, Qt::CaseInsensitive) ||
                entry.genre.contains(trackGenre, Qt::CaseInsensitive)) {
            genrePenalty = 0.35;
        }
    }

    const double energyDiff = qAbs(entry.energy - trackEnergy);
    return bpmDiff * 2.0 + genrePenalty * 0.5 + energyDiff * 0.25;
}

std::optional<VideoPoolEntry> VideoPool::selectBest(double trackBpm,
        const QString& trackGenre,
        double trackEnergy) const {
    ensureLoaded();
    if (m_entries.isEmpty()) {
        return std::nullopt;
    }

    const VideoPoolEntry* best = nullptr;
    double bestScore = std::numeric_limits<double>::max();
    for (const VideoPoolEntry& entry : m_entries) {
        const double score = scoreEntry(entry, trackBpm, trackGenre, trackEnergy);
        if (score < bestScore) {
            bestScore = score;
            best = &entry;
        }
    }
    if (!best) {
        return std::nullopt;
    }
    return *best;
}

double VideoPool::poolLoopTargetSeconds(double audioClockSeconds,
        double rateRatio,
        double deckBpm,
        double loopBpm,
        double loopDurationSeconds) {
    if (loopDurationSeconds <= 0.0 || loopBpm <= 0.0) {
        return 0.0;
    }
    const double rate = rateRatio > 0.0 ? rateRatio : 1.0;
    const double bpm = deckBpm > 0.0 ? deckBpm : loopBpm;
    double target = std::fmod(audioClockSeconds * rate * (bpm / loopBpm),
            loopDurationSeconds);
    if (target < 0.0) {
        target += loopDurationSeconds;
    }
    return target;
}
