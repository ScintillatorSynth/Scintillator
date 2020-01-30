#ifndef SRC_OSC_COMMANDS_DEF_RECEIVE_HPP_
#define SRC_OSC_COMMANDS_DEF_RECEIVE_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class DefReceive : public Command {
public:
    DefReceive(osc::Dispatcher* dispatcher);
    virtual ~DefReceive();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_message message) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_DEF_RECEIVE_HPP_
