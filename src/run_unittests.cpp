#include "LogLevels.hpp"

#include "gflags/gflags.h"
#include "gtest/gtest.h"
#include "spdlog/spdlog.h"

DEFINE_int32(log_level, 6, "Verbosity of logs, 0 most verbose, 6 is off.");

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    ::testing::InitGoogleTest(&argc, argv);
    scin::setGlobalLogLevel(FLAGS_log_level);
    return RUN_ALL_TESTS();
}
