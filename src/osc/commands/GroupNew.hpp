#ifndef SRC_OSC_COMMANDS_GROUP_NEW_HPP_
#define SRC_OSC_COMMANDS_GROUP_NEW_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class GroupNew : public Command {
public:
    GroupNew(osc::Dispatcher* dispatcher);
    virtual ~GroupNew();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_address address) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_GROUP_NEW_HPP_
