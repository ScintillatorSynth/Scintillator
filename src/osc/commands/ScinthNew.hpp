#ifndef SRC_OSC_COMMANDS_SCINTH_NEW_HPP_
#define SRC_OSC_COMMANDS_SCINTH_NEW_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class ScinthNew : public Command {
public:
    ScinthNew(osc::Dispatcher* dispatcher);
    virtual ~ScinthNew();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_address address) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_SCINTH_NEW_HPP_
