#ifndef SRC_OSC_COMMANDS_NODE_FREE_HPP_
#define SRC_OSC_COMMANDS_NODE_FREE_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class NodeFree : public Command {
public:
    NodeFree(osc::Dispatcher* dispatcher);
    virtual ~NodeFree();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_message message) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_NODE_FREE_HPP_
