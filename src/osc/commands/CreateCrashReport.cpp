#include "osc/commands/CreateCrashReport.hpp"

#include "infra/CrashReporter.hpp"
#include "osc/Dispatcher.hpp"

#include <spdlog/spdlog.h>

#include <cstring>

namespace scin { namespace osc { namespace commands {

CreateCrashReport::CreateCrashReport(osc::Dispatcher* dispatcher): Command(dispatcher) {}

CreateCrashReport::~CreateCrashReport() {}

void CreateCrashReport::processMessage(int argc, lo_arg** argv, const char* types, lo_address address) {
    if (m_dispatcher->crashReporter()) {
#if __linux__ || WIN32
        m_dispatcher->crashReporter()->dumpWithoutCrash();
#else
        spdlog::warn("crash report requested but crashpad doesn't support crashes on this platform.");
#endif
    } else {
        spdlog::warn("Crash reporting disabled.");
    }
}

} // namespace commands
} // namespace osc
} // namespace scin
