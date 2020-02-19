#include "Logger.hpp"

#include "av/AVIncludes.hpp"

#include <array>
#include <mutex>

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

namespace spdlog { namespace sinks {

class ErrorSink : public base_sink<std::mutex> {
public:
    ErrorSink(): m_numberOfErrors(0), m_numberOfWarnings(0) {}
    virtual ~ErrorSink() {}

    void getCounts(size_t& warningsOut, size_t& errorsOut) {
        std::lock_guard<std::mutex> lock(mutex_);
        warningsOut = m_numberOfWarnings;
        errorsOut = m_numberOfErrors;
    }

protected:
    void sink_it_(const details::log_msg& msg) override {
        switch (msg.level) {
        case level::level_enum::warn:
            ++m_numberOfWarnings;
            break;

        case level::level_enum::err:
        case level::level_enum::critical:
            ++m_numberOfErrors;
            break;

        default:
            break;
        }
    }

    void flush_() override {}

private:
    size_t m_numberOfErrors;
    size_t m_numberOfWarnings;
};

} // namespace sinks
} // namespace spdlog

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

Logger::Logger() {}

Logger::~Logger() {}

void Logger::initLogging(int level) {
    // Even if other logging is disabled we always keep the ErrorSink at log level warn, so that it can count the
    // incoming log messages with that label or more severe.
    m_errorSink.reset(new spdlog::sinks::ErrorSink());
    m_errorSink->set_level(spdlog::level::level_enum::warn);
    m_consoleSink.reset(new spdlog::sinks::stdout_color_sink_mt());
    m_logger.reset(new spdlog::logger("scin_logger", { m_errorSink, m_consoleSink }));
    // Set the logger main level to get everything, then we can set levels on individual sinks to higher as requested.
    m_logger->set_level(spdlog::level::level_enum::trace);
    spdlog::set_default_logger(m_logger);
    spdlog::set_pattern("[%E.%e] %t [%^%l%$] %v");
    av_log_set_callback(avLogCallback);
    setConsoleLogLevel(level);
}

void Logger::setConsoleLogLevel(int level) {
    if (level < 0 || level > 6) {
        return;
    }

    // We set libav to log at least warnings, to get notified of errors and warnings and update our counts.
    int atLeastWarnings = std::min(level, 3);
    av_log_set_level(spdLevelToAVLevel(static_cast<spdlog::level::level_enum>(atLeastWarnings)));
    m_consoleSink->set_level(static_cast<spdlog::level::level_enum>(level));
    spdlog::info("log level set to {}", level);
}

void Logger::getCounts(size_t& warningsOut, size_t& errorsOut) { m_errorSink->getCounts(warningsOut, errorsOut); }

} // namespace scin
