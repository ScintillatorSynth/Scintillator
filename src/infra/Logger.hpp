#ifndef SRC_INFRA_LOGGER_HPP_
#define SRC_INFRA_LOGGER_HPP_

#include "spdlog/spdlog.h"
#include "spdlog/sinks/base_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <memory>
#include <string>

namespace spdlog { namespace sinks {
class ErrorSink;
}}

namespace scin { namespace infra {

class Logger {
public:
    Logger();
    ~Logger();

    void initLogging(int level);
    void setConsoleLogLevel(int level);

    /*! Returns the number of warnings and errors logged in the application to date.
     */
    void getCounts(size_t& warningsOut, size_t& errorsOut);

    /*! Vulkan uses a different logic to log thread IDs in the validation layer messages, so any thread that handles
     * VUlkan objects can call this static method to log the Vulkan thread ID, allowing for correlation with the spdlog
     * thread IDs.
     *
     * \param threadName The human-readable name to associate with the thread IDs in the logs.
     */
    static void logVulkanThreadID(const std::string& threadName);

private:
    std::shared_ptr<spdlog::sinks::ErrorSink> m_errorSink;
    std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> m_consoleSink;
    std::shared_ptr<spdlog::logger> m_logger;
};

} // namespace infra

} // namespace scin

#endif // SRC_INFRA_LOGGGER_HPP_
