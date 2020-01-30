#ifndef SRC_OSC_COMMANDS_DEF_LOAD_HPP_
#define SRC_OSC_COMMANDS_DEF_LOAD_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class DefLoad : public Command {
public:
    DefLoad(osc::Dispatcher* dispatcher);
    virtual ~DefLoad();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_message message) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_DEF_LOAD_HPP_
