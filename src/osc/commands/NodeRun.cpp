#include "osc/commands/NodeRun.hpp"

#include "osc/Dispatcher.hpp"

namespace scin { namespace osc { namespace commands {

NodeRun::NodeRun(osc::Dispatcher* dispatcher): Command(dispatcher) {}

NodeRun::~NodeRun() {}

void NodeRun::processMessage(int argc, lo_arg** argv, const char* types, lo_message message) {}

} // namespace commands
} // namespace osc
} // namespace scin
