#include "av/Frame.hpp"

#include "av/Buffer.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace av {

Frame::Frame(std::shared_ptr<Buffer> buffer): m_buffer(buffer), m_frame(nullptr) {}

Frame::~Frame() { destroy(); }

bool Frame::create(int width, int height) {
    m_frame = av_frame_alloc();
    if (!m_frame) {
        spdlog::error("Frame failed to create AVFrame");
        return false;
    }
    m_frame->width = width;
    m_frame->height = height;
    m_frame->format = AV_PIX_FMT_RGBA;
    m_frame->buf[0] = m_buffer->addReference();
    return true;
}

void Frame::destroy() {
    av_frame_free(&m_frame);
}

} // namespace av

} // namespace scin
