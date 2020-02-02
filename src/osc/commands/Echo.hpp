#ifndef SRC_OSC_COMMANDS_ECHO_HPP_
#define SRC_OSC_COMMANDS_ECHO_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class Echo : public Command {
public:
    Echo(osc::Dispatcher* dispatcher);
    virtual ~Echo();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_address address) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_ECHO_HPP_
