#include "infra/CrashReporter.hpp"

#include "infra/Version.hpp"

#if __GNUC__ || __clang__
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#include <client/crash_report_database.h>
#include <client/crashpad_client.h>
#include <client/settings.h>
#if __GNUC__ || __clang__
#    pragma GCC diagnostic pop
#endif

#include <fmt/core.h>
#include <spdlog/spdlog.h>
#include <util/misc/capture_context.h>
#include <util/misc/uuid.h>

#include <array>
#include <map>
#include <string>
#include <time.h>
#include <vector>

namespace {

static const char* kGargamelleURL = "https://ggml.scintillatorsynth.org/api/dump";

void logReport(const crashpad::CrashReportDatabase::Report& report) {
    tm* createTime = localtime(&report.creation_time);
    std::array<char, 128> timeBuf;
    strftime(timeBuf.data(), sizeof(timeBuf), "%a %d %b %H:%M:%S %Y", createTime);
    spdlog::info("    id: {}, on: {}, uploaded: {}", report.uuid.ToString(), timeBuf.data(),
                 report.uploaded ? "yes" : "no");
}
} // namespace

namespace scin { namespace infra {

CrashReporter::CrashReporter(const fs::path& handler, const fs::path& database, const fs::path& metrics):
    m_handlerPath(handler),
    m_databasePath(database),
    m_metricsPath(metrics),
    m_client(new crashpad::CrashpadClient()) {}

CrashReporter::~CrashReporter() {}

bool CrashReporter::startCrashHandler() {
    std::map<std::string, std::string> metadata;
    metadata["program"] = "scinsynth";
    metadata["version"] = fmt::format("{}.{}.{}", kScinVersionMajor, kScinVersionMinor, kScinVersionPatch);
    metadata["commit"] = kScinCompleteHash;
    metadata["branch"] = kScinBranch;

#if (WIN32)
    // TODO: crash reporting seems to be working despite the header comments in crashpad_client.h stating that on
    // windows a named IPC pipe call must also be made after startup. Investigate the inconsistency.
    return m_client->StartHandler(base::FilePath(m_handlerPath.wstring()), base::FilePath(m_databasePath.wstring()),
                                  base::FilePath(m_metricsPath.wstring()), kGargamelleURL, metadata,
                                  std::vector<std::string>(), false, false, std::vector<base::FilePath>());
#else
    return m_client->StartHandler(base::FilePath(m_handlerPath.string()), base::FilePath(m_databasePath.string()),
                                  base::FilePath(m_metricsPath.string()), kGargamelleURL, metadata,
                                  std::vector<std::string>(), false, false, std::vector<base::FilePath>());
#endif
}


bool CrashReporter::openDatabase() {
    // Early-out for database already open.
    if (m_database) {
        return true;
    }

    m_database = crashpad::CrashReportDatabase::Initialize(base::FilePath(m_databasePath));
    if (!m_database) {
        spdlog::error("CrashReporter failed to open or create crash database.");
        return false;
    }

    return true;
}

void CrashReporter::closeDatabase() { m_database = nullptr; }

int CrashReporter::logCrashReports() {
    if (!openDatabase())
        return -1;

    std::vector<crashpad::CrashReportDatabase::Report> pendingReports;
    if (m_database->GetPendingReports(&pendingReports) != crashpad::CrashReportDatabase::kNoError) {
        spdlog::error("Failed to enumerate pending reports in the crash database.");
        return -1;
    }
    std::vector<crashpad::CrashReportDatabase::Report> completedReports;
    if (m_database->GetCompletedReports(&completedReports) != crashpad::CrashReportDatabase::kNoError) {
        spdlog::error("Failed to enumerate complete reports in the crash database.");
        return -1;
    }

    auto notUploaded = pendingReports.size();
    if (pendingReports.size() + completedReports.size()) {
        spdlog::info("Crash report database contains {} reports:", pendingReports.size() + completedReports.size());
        for (auto report : pendingReports) {
            logReport(report);
        }
        for (auto report : completedReports) {
            logReport(report);
            if (!report.uploaded) {
                ++notUploaded;
            }
        }
    } else {
        spdlog::info("Crash reports database contains no reports.");
    }
    return notUploaded;
}

bool CrashReporter::uploadCrashReport(const std::string& reportUUID) {
    if (!openDatabase())
        return false;

    crashpad::UUID uuid;
    if (!uuid.InitializeFromString(reportUUID)) {
        spdlog::error("Failed to parse crash report UUID string {}.", reportUUID);
        return false;
    }

    std::vector<crashpad::CrashReportDatabase::Report> pendingReports;
    if (m_database->GetPendingReports(&pendingReports) != crashpad::CrashReportDatabase::kNoError) {
        spdlog::error("Failed to enumerate pending reports in the crash database.");
        return false;
    }

    for (auto report : pendingReports) {
        if (uuid == report.uuid) {
            spdlog::info("Requesting upload for crash report UUID {}.", reportUUID);
            if (m_database->RequestUpload(uuid) != crashpad::CrashReportDatabase::kNoError) {
                spdlog::error("Failed requesting upload for crash report UUID {}.", reportUUID);
                return false;
            }
            return true;
        }
    }

    std::vector<crashpad::CrashReportDatabase::Report> completedReports;
    if (m_database->GetCompletedReports(&completedReports) != crashpad::CrashReportDatabase::kNoError) {
        spdlog::error("Failed to enumerate complete reports in the crash database.");
        return false;
    }

    for (auto report : completedReports) {
        if (uuid == report.uuid) {
            spdlog::info("Requesting upload for crash report UUID {}.", reportUUID);
            if (m_database->RequestUpload(uuid) != crashpad::CrashReportDatabase::kNoError) {
                spdlog::error("Failed requesting upload for crash report UUID {}.", reportUUID);
                return false;
            }
            return true;
        }
    }

    spdlog::error("Crash report UUID {} not found.", reportUUID);
    return false;
}

bool CrashReporter::uploadAllCrashReports() {
    if (!openDatabase())
        return false;

    std::vector<crashpad::CrashReportDatabase::Report> pendingReports;
    if (m_database->GetPendingReports(&pendingReports) != crashpad::CrashReportDatabase::kNoError) {
        spdlog::error("Failed to enumerate pending reports in the crash database.");
        return false;
    }

    for (auto report : pendingReports) {
        if (!report.uploaded) {
            std::string uuid = report.uuid.ToString();
            spdlog::info("Requesting upload for crash report UUID {}.", uuid);
            if (m_database->RequestUpload(report.uuid) != crashpad::CrashReportDatabase::kNoError) {
                spdlog::error("Failed requesting upload for crash report UUID {}.", uuid);
                return false;
            }
        }
    }

    std::vector<crashpad::CrashReportDatabase::Report> completedReports;
    if (m_database->GetCompletedReports(&completedReports) != crashpad::CrashReportDatabase::kNoError) {
        spdlog::error("Failed to enumerate complete reports in the crash database.");
        return false;
    }

    for (auto report : completedReports) {
        if (!report.uploaded) {
            std::string uuid = report.uuid.ToString();
            spdlog::info("Requesting upload for crash report UUID {}.", uuid);
            if (m_database->RequestUpload(report.uuid) != crashpad::CrashReportDatabase::kNoError) {
                spdlog::error("Failed requesting upload for crash report UUID {}.", uuid);
                return false;
            }
        }
    }

    return true;
}

} // namespace infra
} // namespace scin
