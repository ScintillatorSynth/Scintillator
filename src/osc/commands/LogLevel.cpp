#include "osc/commands/LogLevel.hpp"

#include "infra/Logger.hpp"
#include "osc/Dispatcher.hpp"

#include <spdlog/spdlog.h>

namespace scin { namespace osc { namespace commands {

LogLevel::LogLevel(osc::Dispatcher* dispatcher): Command(dispatcher) {}

LogLevel::~LogLevel() {}

void LogLevel::processMessage(int argc, lo_arg** argv, const char* types, lo_address /* address */) {
    if (argc < 1 || types[0] != LO_INT32) {
        spdlog::error("OSC LogLevel got invalid or absent level argument in message.");
        return;
    }
    int32_t logLevel = *reinterpret_cast<int32_t*>(argv[0]);
    if (logLevel < 0 || logLevel > 6) {
        spdlog::error("OSC LogLevel got invalid log level {}", logLevel);
    } else {
        m_dispatcher->logger()->setConsoleLogLevel(logLevel);
    }
}

} // namespace commands
} // namespace osc
} // namespace scin
