#ifndef SRC_OSC_COMMANDS_NODE_RUN_HPP_
#define SRC_OSC_COMMANDS_NODE_RUN_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class NodeRun : public Command {
public:
    NodeRun(osc::Dispatcher* dispatcher);
    virtual ~NodeRun();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_message message) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_NODE_RUN_HPP_
