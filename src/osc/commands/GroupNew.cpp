#include "osc/commands/GroupNew.hpp"

#include "comp/RootNode.hpp"
#include "osc/Dispatcher.hpp"

#include "spdlog/spdlog.h"

#include <vector>

namespace scin { namespace osc { namespace commands {

GroupNew::GroupNew(osc::Dispatcher* dispatcher): Command(dispatcher) {}

GroupNew::~GroupNew() {}

void GroupNew::processMessage(int argc, lo_arg** argv, const char* types, lo_address /* address */) {
    std::vector<std::tuple<int, comp::RootNode::AddAction, int>> groups;
    if (argc % 3) {
        spdlog::warn("OSC groupNew got non-triplet arguments, dropping last elements");
        argc -= (argc % 3);
    }
    for (auto i = 0; i < argc; i += 3) {
        if (types[i] == LO_INT32 && types[i + 1] == LO_INT32 && types[i + 2] == LO_INT32) {
            int actionNumber = *reinterpret_cast<int32_t*>(argv[i + 1]);
            if (actionNumber >= 0 && actionNumber < comp::RootNode::AddAction::kActionCount) {
                groups.emplace_back(std::make_tuple(*reinterpret_cast<int32_t*>(argv[i]),
                            static_cast<comp::RootNode::AddAction>(actionNumber),
                            *reinterpret_cast<int32_t*>(argv[i + 2])));
            } else {
                spdlog::warn("OSC groupNew got bad add action number {}", actionNumber);
            }
        } else {
            spdlog::warn("OSC message groupNew got non-integral type argument");
        }
    }
    m_dispatcher->rootNode()->groupNew(groups);
}

} // namespace commands
} // namespace osc
} // namespace scin
