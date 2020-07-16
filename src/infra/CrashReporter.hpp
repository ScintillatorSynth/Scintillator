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
    /*! CrashReporter constructor.
     *
     * \param databasePath A file path to the crash report database root directory. If it does not exist it will be
     *        created.
     */
    explicit CrashReporter(const std::string& databasePath);

    /*! CrashReporter destructor. Will disable crash reporting when executed, so should be deferred as late as possible
     * in process exit.
     */
    ~CrashReporter();

    /*! Start the external crash handling process. Blocks until it has been successfully started.
     *
     * \return true if the hander was successfully started.
     */
    bool startCrashHandler();

    /*! Opens the crash report database. Any crash report query or settings change will open the database automatically,
     * but it can be opened and closed manually to allow for efficiency with multiple queries.
     *
     * \return true on success, false on failure.
     */
    bool openDatabase();

    /*! Closes the crash report database. It is not required to have the database open to collect crash reports.
     */
    void closeDatabase();

    /*! Reads the current uploads enabled setting from the crash report database.
     *
     * \param A pointer to a boolean where the enabled setting will be stored on return.
     * \return true if the value was successfully retrieved, false if not.
     */
    bool uploadsEnabled(bool* enabled);

    /*! Store a value for uploads enabled in the crash report database.
     * \note Crash Reports can contain personal information, enable automatic upload with caution.
     * \return true if the value was successfully set, false if not.
     */
    bool setUploadsEnabled(bool enabled);

    /*! Print all crash reports in the database to the log at the informational level (2).
     *
     * \return The number of un-uploaded crash reports in the database, or -1 if there was an error.
     */
    int logCrashReports();

    /*! Requests the handler to generate a minidump stack trace and all accessory information in a crash report.
     * Useful for testing the crash reporting system.
     */
    void dumpWithoutCrash();

    /*! Mark the provided crash report as ready for upload by the handler process.
     *
     * \param reuprtUUID The UUID of the crash report to upload. These can be extracted from the log.
     * \return true if the crash report was found and successfully marked for upload, false otherwise.
     */
    bool uploadCrashReport(const std::string& reportUUID);

    /*! Mark all unuploaded crash reports as ready for upload by the handler process.
     *
     * \return true if all reports were successfully marked, false otherwise.
     */
    bool uploadAllCrashReports();

private:
    std::string m_databasePath;

    std::unique_ptr<crashpad::CrashReportDatabase> m_database;
    std::unique_ptr<crashpad::CrashpadClient> m_client;
};

} // namespace infra
} // namespace scin

#endif // SRC_INFRA_CRASH_REPORTER_HPP_
