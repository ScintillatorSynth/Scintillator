#include "LogLevels.hpp"

#include "av/AVIncludes.hpp"

#include "spdlog/spdlog.h"

#include <array>

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

namespace {
// Converts an AVLogLevel enum to a spdlog level enum.
spdlog::level::level_enum avLevelToSpdLevel(int level) {
    switch (level) {
    case AV_LOG_QUIET:
        return spdlog::level::level_enum::off;

    case AV_LOG_PANIC:
    case AV_LOG_FATAL:
        return spdlog::level::level_enum::critical;

    case AV_LOG_ERROR:
        return spdlog::level::level_enum::err;

    case AV_LOG_WARNING:
        return spdlog::level::level_enum::warn;

    case AV_LOG_INFO:
        return spdlog::level::level_enum::info;

    case AV_LOG_VERBOSE:
        return spdlog::level::level_enum::debug;

    case AV_LOG_DEBUG:
        return spdlog::level::level_enum::trace;
    }

    return spdlog::level::level_enum::off;
}

int spdLevelToAVLevel(spdlog::level::level_enum level) {
    switch (level) {
    case spdlog::level::level_enum::off:
        return AV_LOG_QUIET;

    case spdlog::level::level_enum::critical:
        return AV_LOG_PANIC;

    case spdlog::level::level_enum::err:
        return AV_LOG_ERROR;

    case spdlog::level::level_enum::warn:
        return AV_LOG_WARNING;

    case spdlog::level::level_enum::info:
        return AV_LOG_INFO;

    case spdlog::level::level_enum::debug:
        return AV_LOG_VERBOSE;

    case spdlog::level::level_enum::trace:
        return AV_LOG_DEBUG;
    }

    return AV_LOG_QUIET;
}

static void avLogCallback(void* ptr, int level, const char* fmt, va_list vaList) {
    static int printPrefix = 1;
    std::array<char, 1024> buffer;
    av_log_format_line(ptr, level, fmt, vaList, buffer.data(), sizeof(buffer), &printPrefix);
    spdlog::log(avLevelToSpdLevel(level), "libav: {}", buffer.data());
}

} // namespace

namespace scin {

void setGlobalLogLevel(int level) {
    if (level < 0 || level > 6) {
        return;
    }
    av_log_set_callback(avLogCallback);
    av_log_set_level(spdLevelToAVLevel(static_cast<spdlog::level::level_enum>(level)));
    spdlog::set_level(static_cast<spdlog::level::level_enum>(level));
}

} // namespace scin
