#include "osc/commands/LogCrashReports.hpp"

#include "infra/CrashReporter.hpp"
#include "osc/Dispatcher.hpp"

#include <spdlog/spdlog.h>

#include <cstring>

namespace scin { namespace osc { namespace commands {

LogCrashReports::LogCrashReports(osc::Dispatcher* dispatcher): Command(dispatcher) {}

LogCrashReports::~LogCrashReports() {}

void LogCrashReports::processMessage(int argc, lo_arg** argv, const char* types, lo_address address) {
#if defined(SCIN_USE_CRASHPAD)
    m_dispatcher->crashReporter()->logCrashReports();
    m_dispatcher->crashReporter()->closeDatabase();
#else
    spdlog::warn("crash log requested but build has crashpad disabled.");
#endif
    m_dispatcher->respond(address, "/scin_done", "/scin_logCrashReports");
}

} // namespace commands
} // namespace osc
} // namespace scin
