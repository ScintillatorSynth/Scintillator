#ifndef SRC_OSC_COMMANDS_SYNC_HPP_
#define SRC_OSC_COMMANDS_SYNC_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class Sync : public Command {
public:
    Sync(osc::Dispatcher* dispatcher);
    virtual ~Sync();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_address address) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_SYNC_COMMAND_HPP_
