#ifndef SRC_OSC_COMMANDS_VERSION_HPP_
#define SRC_OSC_COMMANDS_VERSION_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class Version : public Command {
public:
    Version(osc::Dispatcher* dispatcher);
    virtual ~Version();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_message message) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_VERSION_HPP_
