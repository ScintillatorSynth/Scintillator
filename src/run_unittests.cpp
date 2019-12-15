#include "gtest/gtest.h"

#include "spdlog/spdlog.h"

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    spdlog::set_level(spdlog::level::level_enum::off);
    return RUN_ALL_TESTS();
}
