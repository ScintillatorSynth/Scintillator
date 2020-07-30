#include "osc/commands/GroupFreeAll.hpp"

#include "comp/Compositor.hpp"
#include "osc/Dispatcher.hpp"

#include "spdlog/spdlog.h"

#include <vector>

namespace scin { namespace osc { namespace commands {

GroupFreeAll::GroupFreeAll(osc::Dispatcher* dispatcher): Command(dispatcher) {}

GroupFreeAll::~GroupFreeAll() {}

void GroupFreeAll::processMessage(int argc, lo_arg** argv, const char* types, lo_address address) {
    m_dispatcher->compositor()->groupFreeAll();
}

} // namespace commands
} // namespace osc
} // namespace scin
