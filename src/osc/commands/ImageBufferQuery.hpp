#ifndef SRC_OSC_COMMANDS_IMAGE_BUFFER_QUERY_HPP_
#define SRC_OSC_COMMANDS_IMAGE_BUFFER_QUERY_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class ImageBufferQuery : public Command {
public:
    ImageBufferQuery(osc::Dispatcher* dispatcher);
    virtual ~ImageBufferQuery();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_address address) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_IMAGE_BUFFER_QUERY_HPP_
