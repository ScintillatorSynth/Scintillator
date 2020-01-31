#include "osc/commands/ScreenShot.hpp"

#include "av/ImageEncoder.hpp"
#include "osc/Dispatcher.hpp"
#include "vulkan/Offscreen.hpp"

#include "spdlog/spdlog.h"

#include <string>

namespace scin { namespace osc { namespace commands {

ScreenShot::ScreenShot(osc::Dispatcher* dispatcher): Command(dispatcher), m_encodersSerial(0) {}

ScreenShot::~ScreenShot() {}

void ScreenShot::processMessage(int argc, lo_arg** argv, const char* types, lo_address address) {
    if (!m_dispatcher->offscreen()) {
        spdlog::error("OSC Screenshot requested on realtime framerate.");
        return;
    }
    if (argc < 1 || types[0] != LO_STRING) {
        spdlog::error("OSC ScreenShot got absent or wrong type path argument.");
        return;
    }
    std::string fileName(reinterpret_cast<const char*>(argv[0]));
    std::string mimeType;
    if (argc > 1 && types[1] == LO_STRING) {
        mimeType = std::string(reinterpret_cast<const char*>(argv[1]));
    }
    std::shared_ptr<av::ImageEncoder> encoder;
    size_t serial = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        serial = ++m_encodersSerial;
        encoder.reset(new av::ImageEncoder(
            m_dispatcher->offscreen()->width(), m_dispatcher->offscreen()->height(),
            [this, address, fileName, serial](bool valid) {
                spdlog::info("Screenshot finished encode of '{}', valid: {}", fileName, valid);
                m_dispatcher->respond(address, "/scin_done", "/scin_nrt_screenShot", fileName.data(), valid);
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_encoders.erase(serial);
                }
            }));
        m_encoders.emplace(std::make_pair(m_encodersSerial, encoder));
        ++m_encodersSerial;
    }
    bool valid = false;
    if (encoder->createFile(fileName, mimeType)) {
        spdlog::info("OSC ScreenShot '{}' enqueued for encode.", fileName);
        valid = true;
        m_dispatcher->offscreen()->addEncoder(encoder);
    } else {
        std::lock_guard<std::mutex> lock(m_mutex);
        spdlog::error("OSC ScreenShot failed to create file '{}' with mime type '{}'", fileName, mimeType);
        m_encoders.erase(serial);
    }
}

} // namespace commands
} // namespace osc
} // namespace scin
