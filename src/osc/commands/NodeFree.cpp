#include "osc/commands/NodeFree.hpp"

#include "comp/RootNode.hpp"
#include "osc/Dispatcher.hpp"

#include "spdlog/spdlog.h"

#include <vector>

namespace scin { namespace osc { namespace commands {

NodeFree::NodeFree(osc::Dispatcher* dispatcher): Command(dispatcher) {}

NodeFree::~NodeFree() {}

void NodeFree::processMessage(int argc, lo_arg** argv, const char* types, lo_address /* address */) {
    std::vector<int> nodes;
    for (int i = 0; i < argc; ++i) {
        if (types[i] == LO_INT32) {
            nodes.emplace_back(*reinterpret_cast<int32_t*>(argv[i]));
        } else {
            spdlog::warn("OSC NodeFree skipping non-integer type at index {}", i);
        }
    }
    m_dispatcher->rootNode()->freeNodes(nodes);
}

} // namespace commands
} // namespace osc
} // namespace scin
