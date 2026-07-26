#pragma once

#include <optional>
#include <QString>
#include <QVector>

// Beat-matched visual loop pool (IDEAS.md fallback chain step 2).
struct VideoPoolEntry {
    QString path;
    double bpm = 0.0;
    QString genre;
    double energy = 0.5; // 0..1
};

class VideoPool {
  public:
    static VideoPool& instance();

    void setDirectoryOverride(const QString& directory);
    QString directory() const;

    void reload();
    QVector<VideoPoolEntry> entries() const;

    std::optional<VideoPoolEntry> selectBest(double trackBpm,
            const QString& trackGenre,
            double trackEnergy = 0.5) const;

    static double poolLoopTargetSeconds(double audioClockSeconds,
            double rateRatio,
            double deckBpm,
            double loopBpm,
            double loopDurationSeconds);

    static double energyLabelToValue(const QString& label);
    static std::optional<VideoPoolEntry> parseFileName(const QString& filePath);
    static std::optional<VideoPoolEntry> parseSidecarJson(const QString& jsonPath);

  private:
    VideoPool() = default;

    void ensureLoaded() const;
    VideoPoolEntry entryFromFile(const QString& filePath) const;
    double scoreEntry(const VideoPoolEntry& entry,
            double trackBpm,
            const QString& trackGenre,
            double trackEnergy) const;

    mutable bool m_loaded = false;
    QString m_directoryOverride;
    mutable QVector<VideoPoolEntry> m_entries;
};
