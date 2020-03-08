#include "osc/commands/NodeSet.hpp"

#include "comp/Compositor.hpp"
#include "osc/Dispatcher.hpp"

#include "spdlog/spdlog.h"

#include <string>
#include <vector>

namespace scin { namespace osc { namespace commands {

NodeSet::NodeSet(osc::Dispatcher* dispatcher): Command(dispatcher) {}

NodeSet::~NodeSet() {}

void NodeSet::processMessage(int argc, lo_arg** argv, const char* types, lo_address address) {
    if (argc < 1 || types[0] != LO_INT32) {
        spdlog::error("OSC NodeSet expecting integer ScinthID as first argument.");
        return;
    }
    int nodeID = *reinterpret_cast<int32_t*>(argv[0]);

    std::vector<std::pair<std::string, float>> namedValues;
    std::vector<std::pair<int, float>> indexedValues;
    if (argc % 2 == 0) {
        spdlog::warn("OSC NodeSet got uneven pairs of control arguments, ignoring last pair");
        --argc;
    }
    for (auto i = 1; i < argc; i += 2) {
        float controlValue = 0;
        if (types[i + 1] == LO_FLOAT) {
            controlValue = *reinterpret_cast<float*>(argv[i + 1]);
        } else if (types[i + 1] == LO_INT32) {
            int32_t value = *reinterpret_cast<int32_t*>(argv[i + 1]);
            controlValue = static_cast<float>(value);
        } else {
            spdlog::warn("OSC NodeSet got non-numeric control value, ignoring pair at index {}", i);
            continue;
        }

        if (types[i] == LO_STRING) {
            namedValues.emplace_back(std::make_pair(std::string(reinterpret_cast<const char*>(argv[i])), controlValue));
        } else if (types[i] == LO_INT32) {
            indexedValues.emplace_back(std::make_pair(*reinterpret_cast<int32_t*>(argv[i]), controlValue));
        } else {
            spdlog::warn("OSC NodeSet got unspoorted type for control ID, ignoring pair at index {}", i);
            continue;
        }
    }

    m_dispatcher->compositor()->setNodeParameters(nodeID, namedValues, indexedValues);
}

} // namespace commands
} // namespace osc
} // namespace scin
