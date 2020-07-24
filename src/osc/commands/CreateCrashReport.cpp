#include "osc/commands/CreateCrashReport.hpp"

#include "infra/CrashReporter.hpp"
#include "osc/Dispatcher.hpp"

#include <spdlog/spdlog.h>

#include <cstring>

namespace scin { namespace osc { namespace commands {

CreateCrashReport::CreateCrashReport(osc::Dispatcher* dispatcher): Command(dispatcher) {}

CreateCrashReport::~CreateCrashReport() {}

void CreateCrashReport::processMessage(int argc, lo_arg** argv, const char* types, lo_address address) {
#if __linux__ && defined(SCIN_USE_CRASHPAD)
    m_dispatcher->crashReporter()->dumpWithoutCrash();
#else
    spdlog::warn("crash report requested but build has crashpad disabled.");
    int* foo = nullptr;
    spdlog::error("deref null: {}", *foo);
#endif
}

} // namespace commands
} // namespace osc
} // namespace scin
