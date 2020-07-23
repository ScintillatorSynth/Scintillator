#ifndef SRC_OSC_COMMANDS_LOG_CRASH_REPORTS_HPP_
#define SRC_OSC_COMMANDS_LOG_CRASH_REPORTS_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class LogCrashReports : public Command {
public:
    LogCrashReports(osc::Dispatcher* dispatcher);
    virtual ~LogCrashReports();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_address address) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_LOG_CRASH_REPORTS_HPP_
