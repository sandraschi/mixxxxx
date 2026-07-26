#include <gtest/gtest.h>

#include "control/oscserver.h"

namespace mixxx {

TEST(OscServerTest, EncodeDecodeFloatRoundTrip) {
    const QByteArray encoded = OscServer::encodeFloatMessage("/deck/1/play", 1.0f);

    QString address;
    QVector<QVariant> args;
    ASSERT_TRUE(OscServer::decodeMessage(encoded, &address, &args));
    EXPECT_EQ(address, QStringLiteral("/deck/1/play"));
    ASSERT_EQ(args.size(), 1);
    EXPECT_FLOAT_EQ(args.at(0).toFloat(), 1.0f);
}

TEST(OscServerTest, DecodeStringArgument) {
    QByteArray data;
    auto appendPadded = [](QByteArray* blob, const QString& text) {
        blob->append(text.toUtf8());
        blob->append('\0');
        while (blob->size() % 4 != 0) {
            blob->append('\0');
        }
    };

    appendPadded(&data, QStringLiteral("/deck/2/LoadTrack"));
    appendPadded(&data, QStringLiteral(",s"));
    appendPadded(&data, QStringLiteral("C:/Music/track.mp3"));

    QString address;
    QVector<QVariant> args;
    ASSERT_TRUE(OscServer::decodeMessage(data, &address, &args));
    EXPECT_EQ(address, QStringLiteral("/deck/2/LoadTrack"));
    ASSERT_EQ(args.size(), 1);
    EXPECT_EQ(args.at(0).toString(), QStringLiteral("C:/Music/track.mp3"));
}

} // namespace mixxx
