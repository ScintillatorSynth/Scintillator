#ifndef SRC_OSC_COMMANDS_STATUS_HPP_
#define SRC_OSC_COMMANDS_STATUS_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class Status : public Command {
public:
    Status(osc::Dispatcher* dispatcher);
    virtual ~Status();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_message message) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_STATUS_HPP_
