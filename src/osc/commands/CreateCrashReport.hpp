#ifndef SRC_OSC_COMMANDS_MINI_DUMP_HPP_
#define SRC_OSC_COMMANDS_MINI_DUMP_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class MiniDump : public Command {
public:
    MiniDump(osc::Dispatcher* dispatcher);
    virtual ~MiniDump();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_address address) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_MINI_DUMP_HPP_
