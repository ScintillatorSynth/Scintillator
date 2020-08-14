#include "osc/commands/GroupTail.hpp"

#include "comp/RootNode.hpp"
#include "osc/Dispatcher.hpp"

#include "spdlog/spdlog.h"

#include <vector>

namespace scin { namespace osc { namespace commands {

GroupTail::GroupTail(osc::Dispatcher* dispatcher): Command(dispatcher) {}

GroupTail::~GroupTail() {}

void GroupTail::processMessage(int argc, lo_arg** argv, const char* types, lo_address /* address */) {
    std::vector<std::pair<int, int>> pairs;
    if (argc % 2) {
        spdlog::warn("OSC groupHead got uneven pairs of arguments, dropping last argument");
        --argc;
    }
    for (auto i = 0; i < argc; i += 2) {
        if (types[i] == LO_INT32 && types[i + 1] == LO_INT32) {
            pairs.emplace_back(
                std::make_pair(*reinterpret_cast<int32_t*>(argv[i]), *reinterpret_cast<int32_t*>(argv[i + 1])));
        } else {
            spdlog::warn("OSC message groupHead got non-integral type argument");
        }
    }
    m_dispatcher->rootNode()->groupTail(pairs);
}

} // namespace commands
} // namespace osc
} // namespace scin
