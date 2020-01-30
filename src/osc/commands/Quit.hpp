#ifndef SRC_OSC_COMMANDS_QUIT_HPP_
#define SRC_OSC_COMMANDS_QUIT_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class Quit : public Command {
public:
    Quit(osc::Dispatcher* dispatcher);
    virtual ~Quit();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_message message) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_QUIT_HPP_
