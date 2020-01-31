#ifndef SRC_OSC_COMMANDS_ADVANCE_FRAME_HPP_
#define SRC_OSC_COMMANDS_ADVANCE_FRAME_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class AdvanceFrame : public Command {
public:
    AdvanceFrame(osc::Dispatcher* dispatcher);
    virtual ~AdvanceFrame();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_address address) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_ADVANCE_FRAME_HPP_
