#include "osc/commands/GroupDeepFree.hpp"

#include "comp/RootNode.hpp"
#include "osc/Dispatcher.hpp"

#include "spdlog/spdlog.h"

#include <vector>

namespace scin { namespace osc { namespace commands {

GroupDeepFree::GroupDeepFree(osc::Dispatcher* dispatcher): Command(dispatcher) {}

GroupDeepFree::~GroupDeepFree() {}

void GroupDeepFree::processMessage(int argc, lo_arg** argv, const char* types, lo_address /* address */) {
    std::vector<int> groupIDs;
    for (auto i = 0; i < argc; ++i) {
        if (types[i] == LO_INT32) {
            groupIDs.emplace_back(*reinterpret_cast<int32_t*>(argv[i]));
        } else {
            spdlog::warn("OSC message groupDeepFree got non-integral type argument");
        }
    }
    m_dispatcher->rootNode()->groupDeepFree(groupIDs);
}

} // namespace commands
} // namespace osc
} // namespace scin
