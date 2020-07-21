#include "infra/CrashReporter.hpp"

#include "infra/Version.hpp"

#include <client/crash_report_database.h>
#include <client/crashpad_client.h>
#include <client/settings.h>
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

CrashReporter::CrashReporter(const std::string& crashpadHandlerPath, const std::string& databasePath):
    m_crashpadHandlerPath(crashpadHandlerPath),
    m_databasePath(databasePath),
    m_client(new crashpad::CrashpadClient()) {
}

CrashReporter::~CrashReporter() {
}

bool CrashReporter::startCrashHandler() {
    std::map<std::string, std::string> metadata;
    metadata["program"] = "scinsynth";
    metadata["version"] = fmt::format("{}.{}.{}", kScinVersionMajor, kScinVersionMinor, kScinVersionPatch);
    metadata["commit"] = kScinCompleteHash;
    metadata["branch"] = kScinBranch;

    return m_client->StartHandler(
            base::FilePath(m_crashpadHandlerPath),
            base::FilePath(m_databasePath),
            base::FilePath(""),  // Metrics path
            kGargamelleURL,
            metadata,
            std::vector<std::string>(),
            false,
            false,
            std::vector<base::FilePath>());
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

void CrashReporter::closeDatabase() {
    m_database = nullptr;
}

bool CrashReporter::uploadsEnabled(bool* enabled) {
    if (!openDatabase()) return false;

    crashpad::Settings* settings = m_database->GetSettings();
    if (settings) {
        return settings->GetUploadsEnabled(enabled);
    } else {
        spdlog::error("failed to retrieve settings object from crash database.");
    }
    return false;
}

bool CrashReporter::setUploadsEnabled(bool enabled) {
    if (!openDatabase()) return false;

    crashpad::Settings* settings = m_database->GetSettings();
    if (settings) {
        return settings->SetUploadsEnabled(enabled);
    } else {
        spdlog::error("failed to retrieve settings object from crash database.");
    }
    return false;
}

void CrashReporter::dumpWithoutCrash() {
    spdlog::info("generating minidump without crash.");
    crashpad::NativeCPUContext context;
    crashpad::CaptureContext(&context);
    m_client->DumpWithoutCrash(&context);
}

int CrashReporter::logCrashReports() {
    if (!openDatabase()) return -1;

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
    if (!openDatabase()) return false;

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
    if (!openDatabase()) return false;

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

