#include "osc/commands/ScinthNew.hpp"

#include "comp/RootNode.hpp"
#include "osc/Dispatcher.hpp"

#include "spdlog/spdlog.h"

#include <cstring>
#include <string>
#include <vector>

namespace scin { namespace osc { namespace commands {

ScinthNew::ScinthNew(osc::Dispatcher* dispatcher): Command(dispatcher) {}

ScinthNew::~ScinthNew() {}

void ScinthNew::processMessage(int argc, lo_arg** argv, const char* types, lo_address /* address */) {
    if (std::strncmp(types, "siii", 4) != 0) {
        spdlog::error("OSC ScinthNew got incomplete or incorrect types");
        return;
    }

    std::string scinthDef(reinterpret_cast<const char*>(argv[0]));
    int32_t node = *reinterpret_cast<int32_t*>(argv[1]);

    int32_t addActionNumber = *reinterpret_cast<int32_t*>(argv[2]);
    if (addActionNumber < 0 || addActionNumber >= scin::comp::RootNode::AddAction::kActionCount) {
        spdlog::error("OSC ScinthNew got bad add action number {}", addActionNumber);
        return;
    }
    scin::comp::RootNode::AddAction addAction = static_cast<scin::comp::RootNode::AddAction>(addActionNumber);

    int32_t targetID = *reinterpret_cast<int32_t*>(argv[3]);

    // Rest of message is optional and is a set of key/value pairs for initial values for the parameters of the Scinth.
    std::vector<std::pair<std::string, float>> namedValues;
    std::vector<std::pair<int, float>> indexedValues;
    if (argc % 2 != 0) {
        spdlog::warn("OSC ScinthNew got uneven pairs of control arguments, ignoring last pair.");
        --argc;
    }
    for (auto i = 4; i < argc; i += 2) {
        float controlValue = 0;
        if (types[i + 1] == LO_FLOAT) {
            controlValue = *reinterpret_cast<float*>(argv[i + 1]);
        } else if (types[i + 1] == LO_INT32) {
            int32_t value = *reinterpret_cast<int32_t*>(argv[i + 1]);
            controlValue = static_cast<float>(value);
        } else {
            spdlog::warn("OSC ScinthNew got non-numeric control value, ignoring pair at index {}", i);
            continue;
        }

        if (types[i] == LO_STRING) {
            namedValues.emplace_back(std::make_pair(std::string(reinterpret_cast<const char*>(argv[i])), controlValue));
        } else if (types[i] == LO_INT32) {
            indexedValues.emplace_back(std::make_pair(*reinterpret_cast<int32_t*>(argv[i]), controlValue));
        } else {
            spdlog::warn("OSC NodeSet got unsupported type for control ID, ignoring pair at index {}", i);
            continue;
        }
    }

    m_dispatcher->rootNode()->scinthNew(scinthDef, node, addAction, targetID, namedValues, indexedValues);
}

} // namespace commands
} // namespace osc
} // namespace scin
