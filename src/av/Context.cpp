#include "av/Context.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace av {

Context::Context(): m_codec(nullptr), m_context(nullptr) {}

Context::~Context() { destroy(); }

bool Context::create(Codec codec) {
    m_codec = avcodec_find_encoder(idForEnum(codec));
    if (!m_codec) {
        spdlog::error("failed creating AV codec.");
        return false;
    }

    m_context = avcodec_alloc_context3(m_codec);
    if (!m_context) {
        spdlog::error("failed creating AV context.");
        return false;
    }

    return true;
}

void Context::destroy() {
    avcodec_free_context(&m_context);
}

AVCodecID Context::idForEnum(Codec codec) {
    switch (codec) {
    case kPNG:
        return AV_CODEC_ID_PNG;
    }

    return AV_CODEC_ID_NONE;
}

} // namespace av

} // nammespace scin
