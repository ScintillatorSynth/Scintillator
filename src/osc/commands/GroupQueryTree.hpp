#ifndef SRC_OSC_COMMANDS_GROUP_QUERY_TREE_HPP_
#define SRC_OSC_COMMANDS_GROUP_QUERY_TREE_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class GroupQueryTree : public Command {
public:
    GroupQueryTree(osc::Dispatcher* dispatcher);
    virtual ~GroupQueryTree();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_address address) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_GROUP_QUERY_TREE_HPP_
