#ifndef SRC_OSC_COMMANDS_NODE_ORDER_HPP_
#define SRC_OSC_COMMANDS_NODE_ORDER_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class NodeOrder : public Command {
public:
    NodeOrder(osc::Dispatcher* dispatcher);
    virtual ~NodeOrder();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_address address) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_NODE_ORDER_HPP_
