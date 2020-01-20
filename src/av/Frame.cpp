#include "av/Frame.hpp"

#include "av/Buffer.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace av {

Frame::Frame(std::shared_ptr<Buffer> buffer): m_buffer(buffer), m_frame(nullptr) {}

Frame::~Frame() { destroy(); }

bool Frame::create() {
    m_frame = av_frame_alloc();
    if (!m_frame) {
        spdlog::error("Frame failed to create AVFrame");
        return false;
    }
    m_frame->data[0] = m_buffer->data();
    m_frame->linesize[0] = m_buffer->width() * 4;
    m_frame->extended_data = m_frame->data;
    m_frame->width = m_buffer->width();
    m_frame->height = m_buffer->height();
    m_frame->format = AV_PIX_FMT_RGBA;
    m_frame->buf[0] = m_buffer->addReference();

    return true;
}

void Frame::destroy() { av_frame_free(&m_frame); }

} // namespace av

} // namespace scin
