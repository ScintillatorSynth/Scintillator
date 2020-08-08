#include "osc/commands/GroupFreeAll.hpp"

#include "comp/RootNode.hpp"
#include "osc/Dispatcher.hpp"

#include "spdlog/spdlog.h"

#include <vector>

namespace scin { namespace osc { namespace commands {

GroupFreeAll::GroupFreeAll(osc::Dispatcher* dispatcher): Command(dispatcher) {}

GroupFreeAll::~GroupFreeAll() {}

void GroupFreeAll::processMessage(int argc, lo_arg** argv, const char* types, lo_address /* address */) {
    std::vector<int> nodeIDs;
    for (auto i = 0; i < argc; ++i) {
        if (types[i] == LO_INT32) {
            nodeIDs.emplace_back(*reinterpret_cast<int32_t*>(argv[i]));
        } else {
            spdlog::warn("OSC message groupFreeAll got non-integral type argument");
        }
    }
    m_dispatcher->rootNode()->nodeFree(nodeIDs);
}

} // namespace commands
} // namespace osc
} // namespace scin
