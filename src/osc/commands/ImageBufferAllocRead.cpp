#include "osc/commands/ImageBufferAllocRead.hpp"

#include "comp/Async.hpp"
#include "osc/Address.hpp"
#include "osc/BlobMessage.hpp"
#include "osc/Dispatcher.hpp"

#include "spdlog/spdlog.h"

#include <string>

namespace scin { namespace osc { namespace commands {

ImageBufferAllocRead::ImageBufferAllocRead(osc::Dispatcher* dispatcher): Command(dispatcher) {}

ImageBufferAllocRead::~ImageBufferAllocRead() {}

void ImageBufferAllocRead::processMessage(int argc, lo_arg** argv, const char* types, lo_address address) {
    if (argc < 4 || std::strncmp("isii", types, 4) != 0) {
        spdlog::error("OSC ImageBufferAllocRead missing/incorrect arguments.");
        return;
    }

    int32_t bufferID = *reinterpret_cast<int32_t*>(argv[0]);
    std::string filePath(reinterpret_cast<const char*>(argv[1]));
    int32_t width = *reinterpret_cast<int32_t*>(argv[2]);
    int32_t height = *reinterpret_cast<int32_t*>(argv[3]);
    std::shared_ptr<BlobMessage> onCompletion;
    if (argc > 4 && types[4] == LO_BLOB) {
        onCompletion.reset(new BlobMessage());
        if (!onCompletion->extract(reinterpret_cast<lo_blob>(argv[4]))) {
            spdlog::error("OSC ImageBufferAllocRead failed to extract blob message argument.");
            m_dispatcher->respond(address, "/scin_done", "/scin_ib_readImage", bufferID);
            return;
        }
    }
    std::shared_ptr<Address> origin(new Address(address));
    m_dispatcher->async()->readImageIntoNewBuffer(
        bufferID, filePath, width, height, [this, origin, onCompletion, bufferID]() {
            if (onCompletion) {
                m_dispatcher->processMessageFrom(origin->get(), onCompletion);
            }
            m_dispatcher->respond(origin->get(), "/scin_done", "/scin_ib_readImage", bufferID);
        });
}

} // namespace commands
} // namespace osc
} // namespace scin
