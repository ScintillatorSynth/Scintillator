#include "osc/commands/DefLoadDir.hpp"

#include "osc/Dispatcher.hpp"

namespace scin { namespace osc { namespace commands {

DefLoadDir::DefLoadDir(osc::Dispatcher* dispatcher): Command(dispatcher) {}

DefLoadDir::~DefLoadDir() {}

void DefLoadDir::processMessage(int argc, lo_arg** argv, const char* types, lo_message message) {}

} // namespace commands
} // namespace osc
} // namespace scin
