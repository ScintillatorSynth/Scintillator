#ifndef SRC_OSC_COMMANDS_NODE_BEFORE_HPP_
#define SRC_OSC_COMMANDS_NODE_BEFORE_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class NodeBefore : public Command {
public:
    NodeBefore(osc::Dispatcher* dispatcher);
    virtual ~NodeBefore();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_address address) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_NODE_BEFORE_HPP_
