#include <gtest/gtest.h>

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "util/importcli.h"

using namespace mixxx;

namespace {

QByteArray seratoField(quint32 fieldId, const QByteArray& value) {
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << fieldId;
    stream << static_cast<quint32>(value.size());
    stream.writeRawData(value.constData(), value.size());
    return data;
}

QByteArray seratoStringField(quint32 fieldId, const QString& value) {
    const auto* utf16 = reinterpret_cast<const quint16*>(value.utf16());
    QByteArray bigEndianUtf16(value.length() * 2, '\0');
    for (int i = 0; i < value.length(); ++i) {
        bigEndianUtf16[i * 2] = static_cast<char>((utf16[i] >> 8) & 0xFF);
        bigEndianUtf16[i * 2 + 1] = static_cast<char>(utf16[i] & 0xFF);
    }
    return seratoField(fieldId, bigEndianUtf16);
}

QByteArray makeSeratoCrateTrack(const QString& location) {
    constexpr quint32 kOtrk = 0x6f74726b;
    constexpr quint32 kPtrk = 0x7074726b;
    QByteArray inner;
    inner.append(seratoStringField(kPtrk, location));
    return seratoField(kOtrk, inner);
}

bool writeSeratoCrate(const QString& path, const QStringList& locations) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    for (const QString& loc : locations) {
        file.write(makeSeratoCrateTrack(loc));
    }
    return true;
}

} // namespace

TEST(ImportCliTest, ParseSeratoCrateLocations) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString cratePath = dir.filePath(QStringLiteral("dani-prep.crate"));
    const QString trackA = QStringLiteral("C:/Users/Dani/Music/House/track-a.mp3");
    const QString trackB = QStringLiteral("D:/Library/Dance/track-b.flac");
    ASSERT_TRUE(writeSeratoCrate(cratePath, {trackA, trackB}));

    QString error;
    const QStringList locations = ImportCli::parseImportLocations(cratePath, &error);
    EXPECT_TRUE(error.isEmpty()) << error.toStdString();
    ASSERT_EQ(locations.size(), 2);
    EXPECT_EQ(locations[0], trackA);
    EXPECT_EQ(locations[1], trackB);
}

TEST(ImportCliTest, ParseSeratoCrateMissingFile) {
    QString error;
    const QStringList locations = ImportCli::parseImportLocations(
            QStringLiteral("Z:/no/such/crate.crate"), &error);
    EXPECT_TRUE(locations.isEmpty());
    EXPECT_FALSE(error.isEmpty());
}

TEST(ImportCliTest, ParseVirtualDjFolder) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString vdjPath = dir.filePath(QStringLiteral("warmup.vdjfolder"));
    QFile file(vdjPath);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
               "<VirtualDJFolder>\n"
               "  <track>C:/Music/intro.wav</track>\n"
               "</VirtualDJFolder>\n");
    file.close();

    QString error;
    const QStringList locations = ImportCli::parseImportLocations(vdjPath, &error);
    ASSERT_EQ(locations.size(), 1);
    EXPECT_EQ(locations[0], QStringLiteral("C:/Music/intro.wav"));
}
