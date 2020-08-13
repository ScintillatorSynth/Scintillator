#include "osc/commands/NodeOrder.hpp"

#include "comp/RootNode.hpp"
#include "osc/Dispatcher.hpp"

#include "spdlog/spdlog.h"

#include <vector>

namespace scin { namespace osc { namespace commands {

NodeOrder::NodeOrder(osc::Dispatcher* dispatcher): Command(dispatcher) {}

NodeOrder::~NodeOrder() {}

void NodeOrder::processMessage(int argc, lo_arg** argv, const char* types, lo_address /* address */) {
    if (argc < 2 || types[0] != LO_INT32 || types[1] != LO_INT32) {
        spdlog::error("OSC nodeOrder expecting at least two integer arguments.");
        return;
    }
    int actionNumber = *reinterpret_cast<int32_t*>(argv[0]);
    if (actionNumber >= 0 && actionNumber < comp::RootNode::AddAction::kActionCount) {
        int targetID = *reinterpret_cast<int32_t*>(argv[1]);
        std::vector<int> nodeIDs;
        for (auto i = 2; i < argc; ++i) {
            if (types[i] == LO_INT32) {
                nodeIDs.emplace_back(*reinterpret_cast<int32_t*>(argv[i]));
            } else {
                spdlog::warn("OSC nodeOrder ignoring non-integral type at index {}", i);
            }
        }
        m_dispatcher->rootNode()->nodeOrder(static_cast<comp::RootNode::AddAction>(actionNumber), targetID, nodeIDs);
    } else {
        spdlog::error("OSC nodeOrder got bad add action number {}", actionNumber);
    }
}

} // namespace commands
} // namespace osc
} // namespace scin
