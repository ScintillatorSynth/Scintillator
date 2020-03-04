#include "osc/commands/ImageBufferQuery.hpp"

#include "comp/Compositor.hpp"
#include "osc/Dispatcher.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace osc { namespace commands {

ImageBufferQuery::ImageBufferQuery(osc::Dispatcher* dispatcher): Command(dispatcher) {}

ImageBufferQuery::~ImageBufferQuery() {}

void ImageBufferQuery::processMessage(int argc, lo_arg** argv, const char* types, lo_address address) {
    lo_message message = lo_message_new();
    for (auto i = 0; i < argc; ++i) {
        if (types[i] != LO_INT32) {
            spdlog::error("OSC ImageBufferQuery expecting all integer imageID arguments.");
            return;
        }
        int imageID = *reinterpret_cast<int32_t*>(argv[i]);
        int size = 0;
        int width = -1;
        int height = -1;
        if (!m_dispatcher->compositor()->queryImage(imageID, size, width, height)) {
            spdlog::warn("OSC imageBufferQuery got query for unknown imageID {}.", imageID);
        }
        lo_message_add_int32(message, imageID);
        lo_message_add_int32(message, size);
        lo_message_add_int32(message, width);
        lo_message_add_int32(message, height);
    }

    // The dispatcher will free the message.
    m_dispatcher->respond(address, "/scin_ib_info", message);
}

} // namespace commands
} // namespace osc
} // namespace scin
