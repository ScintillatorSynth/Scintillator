#ifndef SRC_OSC_COMMANDS_LOG_APPEND_HPP_
#define SRC_OSC_COMMANDS_LOG_APPEND_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class LogAppend : public Command {
public:
    LogAppend(osc::Dispatcher* dispatcher);
    virtual ~LogAppend();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_address address) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_LOG_APPEND_HPP_
