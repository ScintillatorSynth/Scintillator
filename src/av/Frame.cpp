#include "av/Frame.hpp"

#include "av/Buffer.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace av {

Frame::Frame(): m_frame(nullptr) {}

Frame::~Frame() { destroy(); }

bool Frame::createFromBuffer(std::shared_ptr<Buffer> buffer) {
    m_frame = av_frame_alloc();
    if (!m_frame) {
        spdlog::error("Frame failed to create AVFrame");
        return false;
    }
    m_buffer = buffer;
    m_frame->data[0] = m_buffer->data();
    m_frame->linesize[0] = m_buffer->width() * 4;
    m_frame->extended_data = m_frame->data;
    m_frame->width = m_buffer->width();
    m_frame->height = m_buffer->height();
    m_frame->format = AV_PIX_FMT_RGBA;
    m_frame->buf[0] = m_buffer->addReference();

    return true;
}

bool Frame::createEmpty() {
    m_frame = av_frame_alloc();
    return m_frame != nullptr;
}

void Frame::destroy() {
    if (m_frame != nullptr) {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }
}

} // namespace av

} // namespace scin
