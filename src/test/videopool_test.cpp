#include <gtest/gtest.h>

#include <cmath>

#include <QTemporaryDir>
#include <QFile>

#include "video/videopool.h"

TEST(VideoPoolTest, ParseFileNameExtractsMetadata) {
    const auto entry = VideoPool::parseFileName(
            QStringLiteral("D:/loops/128-electronic-high.mp4"));
    ASSERT_TRUE(entry.has_value());
    EXPECT_DOUBLE_EQ(entry->bpm, 128.0);
    EXPECT_EQ(entry->genre, QStringLiteral("electronic"));
    EXPECT_NEAR(entry->energy, 0.85, 0.01);
}

TEST(VideoPoolTest, ParseSidecarJsonUsesCompanionFile) {
    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    const QString videoPath = tempDir.path() + QStringLiteral("/ambient.mp4");
    QFile videoFile(videoPath);
    ASSERT_TRUE(videoFile.open(QIODevice::WriteOnly));
    videoFile.write("fake");
    videoFile.close();

    const QString jsonPath = videoPath + QStringLiteral(".json");
    QFile jsonFile(jsonPath);
    ASSERT_TRUE(jsonFile.open(QIODevice::WriteOnly));
    jsonFile.write(R"({"bpm":140,"genre":"ambient","energy":"medium"})");
    jsonFile.close();

    const auto entry = VideoPool::parseSidecarJson(jsonPath);
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(QFileInfo(entry->path).canonicalFilePath(),
            QFileInfo(videoPath).canonicalFilePath());
    EXPECT_DOUBLE_EQ(entry->bpm, 140.0);
    EXPECT_EQ(entry->genre, QStringLiteral("ambient"));
    EXPECT_NEAR(entry->energy, 0.5, 0.01);
}

TEST(VideoPoolTest, SelectBestPrefersCloserBpmAndGenre) {
    VideoPool& pool = VideoPool::instance();
    pool.setDirectoryOverride(QString{});

    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    auto writeLoop = [&](const QString& name) {
        QFile file(tempDir.path() + QDir::separator() + name);
        EXPECT_TRUE(file.open(QIODevice::WriteOnly));
        file.write("fake");
    };
    writeLoop(QStringLiteral("128-house-low.mp4"));
    writeLoop(QStringLiteral("174-drum-bass-high.mp4"));
    writeLoop(QStringLiteral("130-house-med.mp4"));

    pool.setDirectoryOverride(tempDir.path());
    pool.reload();

    const auto match = pool.selectBest(130.0, QStringLiteral("house"), 0.5);
    ASSERT_TRUE(match.has_value());
    EXPECT_TRUE(match->path.contains(QStringLiteral("130-house-med")));
}

TEST(VideoPoolTest, PoolLoopTargetScalesWithDeckBpm) {
    const double atNative = VideoPool::poolLoopTargetSeconds(4.0, 1.0, 128.0, 128.0, 8.0);
    const double fasterDeck = VideoPool::poolLoopTargetSeconds(4.0, 1.0, 174.0, 128.0, 8.0);
    EXPECT_NEAR(atNative, 4.0, 0.001);
    EXPECT_NEAR(fasterDeck, std::fmod(4.0 * (174.0 / 128.0), 8.0), 0.001);
}

TEST(VideoPoolTest, PoolLoopTargetWrapsDuration) {
    const double wrapped = VideoPool::poolLoopTargetSeconds(10.0, 1.0, 128.0, 128.0, 8.0);
    EXPECT_NEAR(wrapped, 2.0, 0.001);
}
