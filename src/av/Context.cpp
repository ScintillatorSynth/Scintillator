#include "av/Context.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace av {

Frame::Frame() {}

Frame::~Frame() {}

Context::Context(): m_context(nullptr) {}

Context::~Context() { avcodec_free_context(&m_context); }

bool Context::createCodecContext(AVCodecID codecID) {
    const AVCodec* codec = avcodec_find_encoder(codecID);
    if (!codec) {
        spdlog::error("failed creating AV codec.");
        return false;
    }

    m_context = avcodec_alloc_context3(codec);
    if (!m_context) {
        spdlog::error("failed creating AV context.");
        return false;
    }

    return true;
}

Encoder::Encoder() {}

Encoder::~Encoder() {}

std::shared_ptr<Frame> Encoder::getEmptyFrame() {
    std::shared_ptr<Frame> emptyFrame;
    {
        std::lock_guard<std::mutex> lock(m_emptyFramesMutex);
        if (m_emptyFrames.size()) {
            emptyFrame = m_emptyFrames.front();
            m_emptyFrames.pop_front();
        }
    }

    if (!emptyFrame) {
        emptyFrame.reset(new Frame());
    }

    return emptyFrame;
}

PNGEncoder::PNGEncoder() {}

PNGEncoder::~PNGEncoder() {}

bool PNGEncoder::create() { return createCodecContext(AV_CODEC_ID_PNG); }

} // namespace av

} // nammespace scin
