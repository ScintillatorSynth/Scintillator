#ifndef SRC_OSC_COMMANDS_DEF_LOAD_DIR_HPP_
#define SRC_OSC_COMMANDS_DEF_LOAD_DIR_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class DefLoadDir : public Command {
public:
    DefLoadDir(osc::Dispatcher* dispatcher);
    virtual ~DefLoadDir();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_address address) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_DEF_LOAD_DIR_HPP_
