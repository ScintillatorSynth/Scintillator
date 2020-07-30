#ifndef SRC_OSC_COMMANDS_GROUP_FREE_ALL_HPP_
#define SRC_OSC_COMMANDS_GROUP_FREE_ALL_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class GroupFreeAll : public Command {
public:
    GroupFreeAll(osc::Dispatcher* dispatcher);
    virtual ~GroupFreeAll();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_address address) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_GROUP_FREE_ALL_HPP_
