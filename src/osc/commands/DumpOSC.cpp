#include "osc/commands/DumpOSC.hpp"

#include "osc/Dispatcher.hpp"

namespace scin { namespace osc { namespace commands {

DumpOSC::DumpOSC(osc::Dispatcher* dispatcher): Command(dispatcher) {}

DumpOSC::~DumpOSC() {}

void DumpOSC::processMessage(int argc, lo_arg** argv, const char* types, lo_message message) {}

} // namespace commands
} // namespace osc
} // namespace scin
