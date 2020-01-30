#include "osc/commands/Quit.hpp"

#include "osc/Dispatcher.hpp"

namespace scin { namespace osc { namespace commands {

Quit::Quit(osc::Dispatcher* dispatcher): Command(dispatcher) {}

Quit::~Quit() {}

void Quit::processMessage(int argc, lo_arg** argv, const char* types, lo_message message) {
}

} // namespace commands
} // namespace osc
} // namespace scin
