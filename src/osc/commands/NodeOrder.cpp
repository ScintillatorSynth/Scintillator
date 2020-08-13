#include "osc/commands/NodeOrder.hpp"

#include "comp/RootNode.hpp"
#include "osc/Dispatcher.hpp"

#include "spdlog/spdlog.h"

#include <vector>

namespace scin { namespace osc { namespace commands {

NodeOrder::NodeOrder(osc::Dispatcher* dispatcher): Command(dispatcher) {}

NodeOrder::~NodeOrder() {}

void NodeOrder::processMessage(int argc, lo_arg** argv, const char* types, lo_address /* address */) {
#error unwritten
}

} // namespace commands
} // namespace osc
} // namespace scin
