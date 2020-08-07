#include "osc/commands/GroupNew.hpp"

#include "comp/RootNode.hpp"
#include "osc/Dispatcher.hpp"

#include "spdlog/spdlog.h"

#include <vector>

namespace scin { namespace osc { namespace commands {

GroupNew::GroupNew(osc::Dispatcher* dispatcher): Command(dispatcher) {}

GroupNew::~GroupNew() {}

void GroupNew::processMessage(int argc, lo_arg** argv, const char* types, lo_address /* address */) {
    std::vector<int> nodeIDs;
    for (auto i = 0; i < argc; ++i) {
        if (types[i] == LO_INT32) {
            nodeIDs.emplace_back(*reinterpret_cast<int32_t*>(argv[i]));
        } else {
            spdlog::warn("OSC message groupFreeAll got non-integral type argument");
        }
    }
    m_dispatcher->rootNode()->groupNew(nodeIDs);
}

} // namespace commands
} // namespace osc
} // namespace scin
