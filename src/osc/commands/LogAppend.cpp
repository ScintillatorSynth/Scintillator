#include "osc/commands/LogAppend.hpp"

#include "osc/Dispatcher.hpp"

#include "spdlog/spdlog.h"

#include <cstring>

namespace scin { namespace osc { namespace commands {

LogAppend::LogAppend(osc::Dispatcher* dispatcher): Command(dispatcher) {}

LogAppend::~LogAppend() {}

void LogAppend::processMessage(int argc, lo_arg** argv, const char* types, lo_address address) {
    if (std::strncmp(types, "is", 2) != 0) {
        spdlog::error("OSC LogAppend got invalid or absent arguments in message.");
        return;
    }
    int32_t logLevel = *reinterpret_cast<int32_t*>(argv[0]);
    if (logLevel < 0 || logLevel > 6) {
        spdlog::error("OSC LogAppend got invalid log level {}", logLevel);
        return;
    }
    const char* logEntry = reinterpret_cast<const char*>(argv[1]);
    spdlog::log(static_cast<spdlog::level::level_enum>(logLevel), "append: {}", logEntry);
}

} // namespace commands
} // namespace osc
} // namespace scin
