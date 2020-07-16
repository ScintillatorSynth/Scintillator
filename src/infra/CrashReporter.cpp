#include "infra/CrashReporter.hpp"

#include <client/crash_report_database.h>
#include <client/crashpad_client.h>
#include <client/settings.h>
#include <spdlog/spdlog.h>
#include <util/misc/capture_context.h>
#include <util/misc/uuid.h>

#include <array>
#include <map>
#include <string>
#include <time.h>
#include <vector>

namespace {
void logReport(const crashpad::CrashReportDatabase::Report& report) {
    tm* createTime = localtime(&report.creation_time);
    std::array<char, 128> timeBuf;
    strftime(timeBuf.data(), sizeof(timeBuf), "%a %d %b %H:%M:%S %Y", createTime);
    spdlog::info("    uuid: {}, creation time: {}, uploaded: {}", report.uuid.ToString(), timeBuf.data(),
            report.uploaded ? "yes" : "no");
}
} // namespace

namespace scin { namespace infra {

CrashReporter::CrashReporter(const std::string& databasePath): m_databasePath(databasePath),
    m_client(new crashpad::CrashpadClient()) {
}

CrashReporter::~CrashReporter() {
}

bool CrashReporter::openDatabase() {
    m_database = crashpad::CrashReportDatabase::Initialize(base::FilePath(m_databasePath));
    if (!m_database) {
        spdlog::error("CrashReporter failed to open or create crash database.");
        return false;
    }

    std::vector<crashpad::CrashReportDatabase::Report> pendingReports;
    if (m_database->GetPendingReports(&pendingReports) != crashpad::CrashReportDatabase::kNoError) {
        spdlog::error("Failed to enumerate pending reports in the crash database.");
        return false;
    }
    std::vector<crashpad::CrashReportDatabase::Report> completedReports;
    if (m_database->GetCompletedReports(&completedReports) != crashpad::CrashReportDatabase::kNoError) {
        spdlog::error("Failed to enumerate complete reports in the crash database.");
        return false;
    }

    spdlog::info("Opened crash report database at {} with {} reports.", m_databasePath);
    for (auto report : pendingReports) {
        logReport(report);
    }
    auto notUploaded = pendingReports.size();
    for (auto report : completedReports) {
        logReport(report);
        if (!report.uploaded) {
            ++notUploaded;
        }
    }

    bool enabled = false;
    if (!uploadsEnabled(&enabled)) {
        spdlog::error("Failed to retrieve uploads enabled from crash report database.");
        return false;
    }

    if (enabled) {
        spdlog::info("Automatic crash report uploads enabled.");
    } else if (notUploaded) {
        spdlog::warn("There are {} crash reports available for upload. Please consider uploading crash reports.", notUploaded);
    }

    return true;
}

bool CrashReporter::uploadsEnabled(bool* enabled) {
    crashpad::Settings* settings = m_database->GetSettings();
    if (settings) {
        return settings->GetUploadsEnabled(enabled);
    } else {
        spdlog::error("failed to retrieve settings object from crash database.");
    }
    return false;
}

bool CrashReporter::setUploadsEnabled(bool enabled) {
    crashpad::Settings* settings = m_database->GetSettings();
    if (settings) {
        return settings->SetUploadsEnabled(enabled);
    } else {
        spdlog::error("failed to retrieve settings object from crash database.");
    }
    return false;
}

bool CrashReporter::startCrashHandler() {
    return m_client->StartHandler(
            base::FilePath("/home/luken/src/Scintillator/build/install-ext/crashpad/out/Default/crashpad_handler"),
            base::FilePath(m_databasePath),
            base::FilePath("/home/luken/src/Scintillator/build/metrics"),
            "http://127.0.0.1:8080/dump",
            std::map<std::string, std::string>(),
            std::vector<std::string>(),
            false,
            false,
            std::vector<base::FilePath>());
}

void CrashReporter::dumpWithoutCrash() {
    spdlog::info("generating minidump without crash.");
    crashpad::NativeCPUContext context;
    crashpad::CaptureContext(&context);
    m_client->DumpWithoutCrash(&context);
}

} // namespace infra
} // namespace scin

