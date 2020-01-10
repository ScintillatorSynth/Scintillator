#include "LogLevels.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace core {

// We currently just re-use the spdlog log levels. But the enum on the spdlog could change, in which case this mapping
// needs to change. We include asserts here to ensure that if the spdlog levels change the compile will break here,
// meaning that a new mapping from the scinsynth 0-6 levels needs to be devised.
static_assert(spdlog::level::level_enum::trace == static_cast<spdlog::level::level_enum>(0));
static_assert(spdlog::level::level_enum::debug == static_cast<spdlog::level::level_enum>(1));
static_assert(spdlog::level::level_enum::info == static_cast<spdlog::level::level_enum>(2));
static_assert(spdlog::level::level_enum::warn == static_cast<spdlog::level::level_enum>(3));
static_assert(spdlog::level::level_enum::err == static_cast<spdlog::level::level_enum>(4));
static_assert(spdlog::level::level_enum::critical == static_cast<spdlog::level::level_enum>(5));
static_assert(spdlog::level::level_enum::off == static_cast<spdlog::level::level_enum>(6));

void setGlobalLogLevel(int level) {
    if (level >= 0 && level <= 6) {
        spdlog::set_level(static_cast<spdlog::level::level_enum>(level));
    }
}

} // namespace core

} // namespace scin
