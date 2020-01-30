#include "osc/commands/DefLoad.hpp"

#include "osc/Dispatcher.hpp"

namespace scin { namespace osc { namespace commands {

DefLoad::DefLoad(osc::Dispatcher* dispatcher): Command(dispatcher) {}

DefLoad::~DefLoad() {}

void DefLoad::processMessage(int argc, lo_arg** argv, const char* types, lo_message message) {}

} // namespace commands
} // namespace osc
} // namespace scin
