#ifndef SRC_OSC_COMMANDS_GROUP_DUMP_TREE_HPP_
#define SRC_OSC_COMMANDS_GROUP_DUMP_TREE_HPP_

#include "osc/commands/Command.hpp"

#include <string>

namespace scin { namespace osc { namespace commands {

class GroupDumpTree : public Command {
public:
    GroupDumpTree(osc::Dispatcher* dispatcher);
    virtual ~GroupDumpTree();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_address address) override;

private:
    void appendDump(int groupID, bool includeParams, std::string indent, std::string& dump);
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_GROUP_DUMP_TREE_HPP_
