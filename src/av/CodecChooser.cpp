#include "CodecChooser.hpp"

#include "spdlog/spdlog.h"

namespace {
const std::vector<const char*> listOfShit = { "a", "b", "c" };
}

namespace scin { namespace av {

CodecChooser::CodecChooser() {}

CodecChooser::~CodecChooser() {}

/*
bool CodecChooser::buildSupportedList() {

    std::vector<const AVCodec*> encoders;
    std::vector<const AVCodec*> decoders;

    const AVCodec* codec = nullptr;
    void* it = nullptr;
    while (codec = av_codec_iterate(&it)) {
        if (avcodec_find_encoder(codec->id)) {
            encoders.push_back(codec);
        }
        if (avcodec_find_decoder(codec->id)) {
            decoders.push_back(codec);
        }
    }

    // Determine pairs of supported output formats by iterating over all output formats and asking each of them if
    // each of the encoder codecs are supported.
    const AVOutputFormat* outputFormat = nullptr;
    it = nullptr;
    while (outputFormat = av_muxer_iterate(&it)) {
        std::vector<const AVCodec*> supportedCodecs;
        std::string names;
        for (auto codec : encoders) {
            if (avformat_query_codec(outputFormat, codec->id, FF_COMPLIANCE_STRICT) == 1) {
                supportedCodecs.push_back(codec);
                names += fmt::format("{} ", codec->name);
            }
        }
        spdlog::debug("enumerating supported codecs for output format {}: {}", outputFormat->name, names);
//        m_outputFormatSupport.emplace(std::string(outputFormat->name), supportedCodecs);
    }

    AVOutputFormat* fmt = av_guess_format(nullptr, "foo.png", nullptr);
    if (fmt) {
        spdlog::info("png output format name: {}", fmt->name);
    } else {
        spdlog::info("no png output format :(");
    }

    return true;
}
*/

} // namespace vk

} // namespace scin
