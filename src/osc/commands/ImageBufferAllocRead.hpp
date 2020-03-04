#ifndef SRC_OSC_COMMANDS_IMAGE_BUFFER_ALLOC_READ_HPP_
#define SRC_OSC_COMMANDS_IMAGE_BUFFER_ALLOC_READ_HPP_

#include "osc/commands/Command.hpp"

namespace scin { namespace osc { namespace commands {

class ImageBufferAllocRead : public Command {
public:
    ImageBufferAllocRead(osc::Dispatcher* dispatcher);
    virtual ~ImageBufferAllocRead();

    void processMessage(int argc, lo_arg** argv, const char* types, lo_address address) override;
};

} // namespace commands
} // namespace osc
} // namespace scin

#endif // SRC_OSC_COMMANDS_IMAGE_BUFFER_ALLOC_READ_HPP_
