#ifndef SRC_OSC_COMMANDS_NODE_SET_HPP_
#define SRC_OSC_COMMANDS_NODE_SET_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class NodeSet : public Command {
public:
    NodeSet(osc::Dispatcher* dispatcher);
    virtual ~NodeSet();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_address address) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_NODE_SET_HPP_
