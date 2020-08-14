#include "osc/commands/GroupDumpTree.hpp"

#include "comp/Node.hpp"
#include "comp/RootNode.hpp"
#include "osc/Dispatcher.hpp"

#include "spdlog/spdlog.h"

#include <string>
#include <vector>

namespace scin { namespace osc { namespace commands {

GroupDumpTree::GroupDumpTree(osc::Dispatcher* dispatcher): Command(dispatcher) {}

GroupDumpTree::~GroupDumpTree() {}

void GroupDumpTree::processMessage(int argc, lo_arg** argv, const char* types, lo_address /* address */) {
    std::vector<std::pair<int, int>> pairs;
    if (argc % 2) {
        spdlog::warn("OSC groupDumpTree got uneven pairs of arguments, dropping last argument");
        --argc;
    }
    std::string dump;
    for (auto i = 0; i < argc; i += 2) {
        if (types[i] == LO_INT32 && types[i + 1] == LO_INT32) {
            int groupID = *reinterpret_cast<int32_t*>(argv[i]);
            bool includeParams = *reinterpret_cast<int32_t*>(argv[i + 1]);
            dump += fmt::format("\nNODE TREE ScinGroup {}\n", groupID);
            appendDump(groupID, includeParams, "    ", dump);
        } else {
            spdlog::warn("OSC message groupDumpTree got non-integral type argument");
        }
    }

    spdlog::info(dump);
}

void GroupDumpTree::appendDump(int groupID, bool includeParams, std::string indent, std::string& dump) {
    std::vector<comp::Node::NodeState> nodes;
    m_dispatcher->rootNode()->groupQueryTree(groupID, nodes);
    for (auto state : nodes) {
        dump += fmt::format("{}{} {}\n", indent, state.nodeID, state.name);
        if (state.numberOfChildren > 0) {
            appendDump(state.nodeID, includeParams, indent + "    ", dump);
        } else if (includeParams && state.numberOfChildren == -1) {
            for (auto param : state.controlValues) {
                dump += fmt::format("    {}{}: {}\n", indent, param.first, param.second);
            }
        }
    }
}

} // namespace commands
} // namespace osc
} // namespace scin
