#include "osc/commands/DefFree.hpp"

#include "comp/Compositor.hpp"
#include "base/Archetypes.hpp"
#include "osc/Dispatcher.hpp"

#include "spdlog/spdlog.h"

#include <string>
#include <vector>

namespace scin { namespace osc { namespace commands {

DefFree::DefFree(osc::Dispatcher* dispatcher): Command(dispatcher) {}

DefFree::~DefFree() {}

void DefFree::processMessage(int argc, lo_arg** argv, const char* types, lo_address address) {
    std::vector<std::string> names;
    for (int i = 0; i < argc; ++i) {
        if (types[i] == LO_STRING) {
            names.emplace_back(std::string(reinterpret_cast<const char*>(argv[i])));
        } else {
            spdlog::warn("OSC DefFree skipping non-string argument at index {}", i);
        }
    }
    m_dispatcher->archetypes()->freeAbstractScinthDefs(names);
    m_dispatcher->compositor()->freeScinthDefs(names);
}

} // namespace commands
} // namespace osc
} // namespace scin
