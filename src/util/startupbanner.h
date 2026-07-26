#pragma once

class CmdlineArgs;

namespace mixxx {

class StartupBanner {
  public:
    /// Print a compact Mixxxxx fork banner to stderr (ASCII art + feature list).
    static void print(const CmdlineArgs& args);
};

} // namespace mixxx
