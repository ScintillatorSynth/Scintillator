#ifndef SRC_OSC_COMMANDS_NOTIFY_HPP_
#define SRC_OSC_COMMANDS_NOTIFY_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class Notify : public Command {
public:
    Notify(osc::Dispatcher* dispatcher);
    virtual ~Notify();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_address address) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_NOTIFY_HPP_
