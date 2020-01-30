#ifndef SRC_OSC_COMMANDS_SCREEN_SHOT_HPP_
#define SRC_OSC_COMMANDS_SCREEN_SHOT_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class ScreenShot : public Command {
public:
    ScreenShot(osc::Dispatcher* dispatcher);
    virtual ~ScreenShot();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_message message) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_SCREEN_SHOT_HPP_
