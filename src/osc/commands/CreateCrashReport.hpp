#ifndef SRC_OSC_COMMANDS_CREATE_CRASH_REPORT_HPP_
#define SRC_OSC_COMMANDS_CREATE_CRASH_REPORT_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class CreateCrashReport : public Command {
public:
    CreateCrashReport(osc::Dispatcher* dispatcher);
    virtual ~CreateCrashReport();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_address address) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_CREATE_CRASH_REPORT_HPP_
