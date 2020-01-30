#include "osc/commands/DefReceive.hpp"

#include "osc/Dispatcher.hpp"

namespace scin { namespace osc { namespace commands {

DefReceive::DefReceive(osc::Dispatcher* dispatcher): Command(dispatcher) {}

DefReceive::~DefReceive() {}

void DefReceive::processMessage(int argc, lo_arg** argv, const char* types, lo_message message) {}

} // namespace commands
} // namespace osc
} // namespace scin
