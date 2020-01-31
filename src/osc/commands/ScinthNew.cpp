#include "osc/commands/ScinthNew.hpp"

#include "Compositor.hpp"
#include "osc/Dispatcher.hpp"

#include "spdlog/spdlog.h"

#include <cstring>
#include <string>

namespace scin { namespace osc { namespace commands {

ScinthNew::ScinthNew(osc::Dispatcher* dispatcher): Command(dispatcher) {}

ScinthNew::~ScinthNew() {}

void ScinthNew::processMessage(int argc, lo_arg** argv, const char* types, lo_address address) {
    if (std::strncmp(types, "si", 2) != 0) {
        spdlog::error("OSC ScinthNew got incomplete or incorrect types");
        return;
    }
    std::string scinthDef(reinterpret_cast<const char*>(argv[0]));
    int32_t node = *reinterpret_cast<int32_t*>(argv[1]);
    // TODO: handle rest of message in terms of placement and group support.
    m_dispatcher->compositor()->cue(scinthDef, node);
}

} // namespace commands
} // namespace osc
} // namespace scin
