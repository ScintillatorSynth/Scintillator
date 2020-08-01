#include "osc/commands/DumpOSC.hpp"

#include "osc/Dispatcher.hpp"

#include "spdlog/spdlog.h"

#include <cstring>

namespace scin { namespace osc { namespace commands {

DumpOSC::DumpOSC(osc::Dispatcher* dispatcher): Command(dispatcher) {}

DumpOSC::~DumpOSC() {}

void DumpOSC::processMessage(int argc, lo_arg** argv, const char* types, lo_address /* address */) {
    if (argc < 1 || std::strncmp(types, "i", 1) != 0) {
        spdlog::error("DumpOSC got wrong argument types {}", types);
        return;
    }
    int32_t enableDump = *reinterpret_cast<int32_t*>(argv[0]);
    m_dispatcher->setDumpOSC(enableDump != 0);
}

} // namespace commands
} // namespace osc
} // namespace scin
