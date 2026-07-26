#include <gtest/gtest.h>

#include "util/exportcli.h"

using namespace mixxx;

TEST(ExportCliTest, ParseExportFormat) {
    ExportFormat format = ExportFormat::Engine;
    ASSERT_TRUE(ExportCli::parseExportFormat(QStringLiteral("engine"), &format));
    EXPECT_EQ(format, ExportFormat::Engine);

    ASSERT_TRUE(ExportCli::parseExportFormat(QStringLiteral("vdj"), &format));
    EXPECT_EQ(format, ExportFormat::VirtualDj);

    ASSERT_TRUE(ExportCli::parseExportFormat(QStringLiteral("serato"), &format));
    EXPECT_EQ(format, ExportFormat::Serato);
}

TEST(ExportCliTest, ParseExportFormatRejectsUnknown) {
    ExportFormat format = ExportFormat::Engine;
    QString error;
    EXPECT_FALSE(ExportCli::parseExportFormat(
            QStringLiteral("pioneer"), &format, &error));
    EXPECT_FALSE(error.isEmpty());
}
