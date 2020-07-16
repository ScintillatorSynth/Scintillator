#ifndef SRC_INFRA_CRASH_REPORTER_HPP_
#define SRC_INFRA_CRASH_REPORTER_HPP_

#include <memory>

namespace crashpad {
class CrashpadClient;
}

namespace scin { namespace infra {

/*! Uses Crashpad to launch an out-of-process crash monitoring executable, which can collect forensic information
 * automatically on detection of a crash or also upon request. Crashes are stored in a local database and can be
 * uploaded to the crash telemetry server, Gargamelle.
 */
class CrashReporter {
public:
    CrashReporter();
    ~CrashReporter();

    bool startCrashHandler();
    void stopCrashHandler();

private:
    std::unique_ptr<crashpad::CrashpadClient> m_client;
};

} // namespace infra
} // namespace scin

#endif // SRC_INFRA_CRASH_REPORTER_HPP_
