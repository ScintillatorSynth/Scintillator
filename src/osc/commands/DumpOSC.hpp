#ifndef SRC_OSC_COMMANDS_DUMP_OSC_HPP_
#define SRC_OSC_COMMANDS_DUMP_OSC_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class DumpOSC : public Command {
public:
    DumpOSC(osc::Dispatcher* dispatcher);
    virtual ~DumpOSC();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_message message) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_DUMP_OSC_HPP_
