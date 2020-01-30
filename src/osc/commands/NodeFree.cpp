#include "osc/commands/NodeFree.hpp"

#include "osc/Dispatcher.hpp"

namespace scin { namespace osc { namespace commands {

NodeFree::NodeFree(osc::Dispatcher* dispatcher): Command(dispatcher) {}

NodeFree::~NodeFree() {}

void NodeFree::processMessage(int argc, lo_arg** argv, const char* types, lo_message message) {}

} // namespace commands
} // namespace osc
} // namespace scin
