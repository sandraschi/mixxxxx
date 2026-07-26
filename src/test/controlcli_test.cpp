#include <gtest/gtest.h>

#include "util/controlcli.h"

namespace mixxx {

TEST(ControlCliTest, ParseSetControlAssignment) {
    QString group;
    QString item;
    double value = 0.0;

    ASSERT_TRUE(ControlCli::parseSetControlAssignment(
            QStringLiteral("[Channel1],play=1"), &group, &item, &value));
    EXPECT_EQ(group, QStringLiteral("[Channel1]"));
    EXPECT_EQ(item, QStringLiteral("play"));
    EXPECT_DOUBLE_EQ(value, 1.0);

    ASSERT_TRUE(ControlCli::parseSetControlAssignment(
            QStringLiteral("[Mixer],crossfader=-0.5"), &group, &item, &value));
    EXPECT_EQ(group, QStringLiteral("[Mixer]"));
    EXPECT_EQ(item, QStringLiteral("crossfader"));
    EXPECT_DOUBLE_EQ(value, -0.5);
}

TEST(ControlCliTest, ParseSetControlAssignmentRejectsBadInput) {
    QString group;
    QString item;
    double value = 0.0;
    QString error;

    EXPECT_FALSE(ControlCli::parseSetControlAssignment(
            QStringLiteral("Channel1,play=1"), &group, &item, &value, &error));
    EXPECT_FALSE(error.isEmpty());

    EXPECT_FALSE(ControlCli::parseSetControlAssignment(
            QStringLiteral("[Channel1],play=not-a-number"), &group, &item, &value, &error));
    EXPECT_FALSE(error.isEmpty());
}

} // namespace mixxx
