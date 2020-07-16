#ifndef SRC_INFRA_CRASH_REPORTER_HPP_
#define SRC_INFRA_CRASH_REPORTER_HPP_

#include <memory>
#include <string>

namespace crashpad {
class CrashpadClient;
class CrashReportDatabase;
}

namespace scin { namespace infra {

/*! Uses Crashpad to launch an out-of-process crash monitoring executable, which can collect forensic information
 * automatically on detection of a crash or also upon request. Crashes are stored in a local database and can be
 * uploaded to the crash telemetry server, Gargamelle.
 */
class CrashReporter {
public:
    CrashReporter(const std::string& databasePath);
    ~CrashReporter();

    bool openDatabase();
    bool uploadsEnabled(bool* enabled);
    bool setUploadsEnabled(bool enabled);

    bool startCrashHandler();

    void dumpWithoutCrash();

private:
    std::string m_databasePath;

    std::unique_ptr<crashpad::CrashReportDatabase> m_database;
    std::unique_ptr<crashpad::CrashpadClient> m_client;
};

} // namespace infra
} // namespace scin

#endif // SRC_INFRA_CRASH_REPORTER_HPP_
