#include "osc/commands/CreateCrashReport.hpp"

#include "infra/CrashReporter.hpp"
#include "osc/Dispatcher.hpp"

#include <spdlog/spdlog.h>

#include <cstring>

namespace scin { namespace osc { namespace commands {

CreateCrashReport::CreateCrashReport(osc::Dispatcher* dispatcher): Command(dispatcher) {}

CreateCrashReport::~CreateCrashReport() {}

void CreateCrashReport::processMessage(int argc, lo_arg** argv, const char* types, lo_address address) {
    m_dispatcher->crashReporter()->dumpWithoutCrash();
}

} // namespace commands
} // namespace osc
} // namespace scin
