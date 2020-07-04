#include "gflags/gflags.h"
#include "gtest/gtest.h"
#include "spdlog/spdlog.h"

DEFINE_int32(logLevel, 6, "Verbosity of logs, 0 most verbose, 6 is off.");

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    ::testing::InitGoogleTest(&argc, argv);
    spdlog::set_level(static_cast<spdlog::level::level_enum>(FLAGS_logLevel));
    return RUN_ALL_TESTS();
}
