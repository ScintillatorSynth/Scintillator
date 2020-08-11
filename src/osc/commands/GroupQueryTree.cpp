#include "osc/commands/GroupQueryTree.hpp"

#include "comp/Node.hpp"
#include "comp/RootNode.hpp"
#include "osc/Dispatcher.hpp"

#include "spdlog/spdlog.h"

#include <memory>
#include <vector>

namespace scin { namespace osc { namespace commands {

GroupQueryTree::GroupQueryTree(osc::Dispatcher* dispatcher): Command(dispatcher) {}

GroupQueryTree::~GroupQueryTree() {}

void GroupQueryTree::processMessage(int argc, lo_arg** argv, const char* types, lo_address address) {
    if (argc % 2) {
        spdlog::warn("OSC groupQueryTree got uneven pairs of arguments, dropping last argument");
        --argc;
    }
    for (auto i = 0; i < argc; i += 2) {
        if (types[i] == LO_INT32 && types[i + 1] == LO_INT32) {
            int groupID = *reinterpret_cast<int32_t*>(argv[i]);
            bool includeParams = (*reinterpret_cast<int32_t*>(argv[i + 1])) != 0;

            std::vector<comp::Node::NodeState> nodes;
            m_dispatcher->rootNode()->groupQueryTree(groupID, nodes);

            lo_message message = lo_message_new();
            if (includeParams) {
                lo_message_add_int32(message, 1);
            } else {
                lo_message_add_int32(message, 0);
            }
            lo_message_add_int32(message, groupID);
            lo_message_add_int32(message, static_cast<int32_t>(nodes.size()));
            for (auto state : nodes) {
                lo_message_add_int32(message, state.nodeID);
                lo_message_add_int32(message, state.numberOfChildren);
                if (state.numberOfChildren == -1) {
                    lo_message_add_string(message, state.name.data());
                    if (includeParams) {
                        for (auto param : state.controlValues) {
                            lo_message_add_string(message, param.first.data());
                            lo_message_add_float(message, param.second);
                        }
                    }
                }
            }

            m_dispatcher->respond(address, "/scin_g_queryTree.reply", message);
        } else {
            spdlog::warn("OSC message queryTree got non-integral type argument, ignoring");
        }
    }
}

} // namespace commands
} // namespace osc
} // namespace scin
