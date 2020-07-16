#include "osc/commands/UploadCrashReport.hpp"

#include "infra/CrashReporter.hpp"
#include "osc/Dispatcher.hpp"

#include <spdlog/spdlog.h>

#include <string>

namespace scin { namespace osc { namespace commands {

UploadCrashReport::UploadCrashReport(osc::Dispatcher* dispatcher): Command(dispatcher) {}

UploadCrashReport::~UploadCrashReport() {}

void UploadCrashReport::processMessage(int argc, lo_arg** argv, const char* types, lo_address address) {
    if (argc < 1 || types[0] != LO_STRING) {
        spdlog::error("OSC UploadCrashReport absent or invalid argument in message.");
        return;
    }

    std::string uuid(reinterpret_cast<const char*>(argv[0]));
    if (uuid == "all") {
        m_dispatcher->crashReporter()->uploadAllCrashReports();
    } else {
        m_dispatcher->crashReporter()->uploadCrashReport(uuid);
    }

    m_dispatcher->respond(address, "/scin_done", "/scin_uploadCrashReport");
}

} // namespace commands
} // namespace osc
} // namespace scin
