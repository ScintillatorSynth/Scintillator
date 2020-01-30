#ifndef SRC_OSC_COMMANDS_DEF_FREE_HPP_
#define SRC_OSC_COMMANDS_DEF_FREE_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class DefFree : public Command {
public:
    DefFree(osc::Dispatcher* dispatcher);
    virtual ~DefFree();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_message message) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_DEF_FREE_HPP_
