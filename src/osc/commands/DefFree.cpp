#include "osc/commands/DefFree.hpp"

#include "osc/Dispatcher.hpp"

namespace scin { namespace osc { namespace commands {

DefFree::DefFree(osc::Dispatcher* dispatcher): Command(dispatcher) {}

DefFree::~DefFree() {}

void DefFree::processMessage(int argc, lo_arg** argv, const char* types, lo_message message) {

}

} // namespace commands
} // namespace osc
} // namespace scin
