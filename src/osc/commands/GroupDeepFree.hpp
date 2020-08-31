#ifndef SRC_OSC_COMMANDS_GROUP_DEEP_FREE_HPP_
#define SRC_OSC_COMMANDS_GROUP_DEEP_FREE_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class GroupDeepFree : public Command {
public:
    GroupDeepFree(osc::Dispatcher* dispatcher);
    virtual ~GroupDeepFree();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_address address) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_GROUP_DEEP_FREE_HPP_
