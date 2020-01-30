#include "osc/commands/LogLevel.hpp"

#include "osc/Dispatcher.hpp"

namespace scin { namespace osc { namespace commands {

LogLevel::LogLevel(osc::Dispatcher* dispatcher): Command(dispatcher) {}

LogLevel::~LogLevel() {}

void LogLevel::processMessage(int argc, lo_arg** argv, const char* types, lo_message message) {
}

} // namespace commands
} // namespace osc
} // namespace scin
