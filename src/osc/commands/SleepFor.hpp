#ifndef SRC_OSC_COMMANDS_SLEEP_FOR_HPP_
#define SRC_OSC_COMMANDS_SLEEP_FOR_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class SleepFor : public Command {
public:
    SleepFor(osc::Dispatcher* dispatcher);
    virtual ~SleepFor();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_address address) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_SLEEP_FOR_HPP_
