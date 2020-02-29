#ifndef SRC_INFRA_LOGGER_HPP_
#define SRC_INFRA_LOGGER_HPP_

#include "spdlog/spdlog.h"
#include "spdlog/sinks/base_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <memory>

namespace spdlog { namespace sinks {
class ErrorSink;
}}

namespace scin {

namespace infra {

class Logger {
public:
    Logger();
    ~Logger();

    void initLogging(int level);
    void setConsoleLogLevel(int level);

    /*! Returns the number of warnings and errors logged in the application to date.
     */
    void getCounts(size_t& warningsOut, size_t& errorsOut);

private:
    std::shared_ptr<spdlog::sinks::ErrorSink> m_errorSink;
    std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> m_consoleSink;
    std::shared_ptr<spdlog::logger> m_logger;
};

} // namespace infra

} // namespace scin

#endif // SRC_INFRA_LOGGGER_HPP_
