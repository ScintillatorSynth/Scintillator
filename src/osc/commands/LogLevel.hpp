#ifndef SRC_OSC_COMMANDS_LOG_LEVEL_HPP_
#define SRC_OSC_COMMANDS_LOG_LEVEL_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class LogLevel : public Command {
public:
    LogLevel(osc::Dispatcher* dispatcher);
    virtual ~LogLevel();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_message message) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_LOG_LEVEL_HPP_
