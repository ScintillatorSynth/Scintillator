#ifndef SRC_OSC_COMMANDS_NODE_AFTER_HPP_
#define SRC_OSC_COMMANDS_NODE_AFTER_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class NodeAfter : public Command {
public:
    NodeAfter(osc::Dispatcher* dispatcher);
    virtual ~NodeAfter();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_address address) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_NODE_AFTER_HPP_
