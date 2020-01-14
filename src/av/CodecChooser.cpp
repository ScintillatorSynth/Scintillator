#include "CodecChooser.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace av {

CodecChooser::CodecChooser() {}

CodecChooser::~CodecChooser() {}

// typeTag format:
// containerName<>codecName
// if < is present we support decoding, if > is present we support encoding (need at least one).

// This one is for writing files. We also build one for existing media that handles reading files.
std::string CodecChooser::getTypeTag(const std::string& typeTag, const std::string& mimeType) {
    std::string containerName = containerNameFromTag(typeTag);
    const char* nameString = containerName.size() ? containerName.data() : nullptr;
    const char* mimeString = mimeType.size() ? mimeType.data() : nullptr;
    AVOutputFormat* outputFormat = av_guess_format(nameString, nullptr, mimeString);
    if (outputFormat) {
        std::string tag(outputFormat->name);
        std::string codecName = codecNameFromTag(typeTag);
        const char* codecString = codecName.size() ? codecName.data() : nullptr;
        AVCodecID codecID = av_guess_codec(outputFormat, codecString, nullptr, mimeString, AVMEDIA_TYPE_UNKNOWN);
        bool codecFound = false;
        AVCodec* decoder = avcodec_find_decoder(codecID);
        if (decoder) {
            tag += "<";
            codecFound = true;
        }
        AVCodec* encoder = avcodec_find_encoder(codecID);
        if (encoder) {
            tag += ">";
            codecFound = true;
        }
        if (!codecFound) {
            spdlog::warn("getTypeTag with typeTag {}, mimeType {} found container {} but no codecs.", typeTag, mimeType,
                         outputFormat->name);
            return std::string();
        }
        if (decoder && encoder && std::strcmp(decoder->name, encoder->name) != 0) {
            spdlog::critical("container {}, decoder {}, and encoder {} have different names!", outputFormat->name,
                             decoder->name, encoder->name);
            return std::string();
        }

        tag += std::string(decoder->name);
        spdlog::info("getTypeTag with typeTag {}, mimeType {} built tag {}", typeTag, mimeType, tag);
        return tag;
    }

    spdlog::info("getTypeTag with typeTag {}, mimeType {} found no supported output format", typeTag, mimeType);
    return std::string();
}

std::string CodecChooser::getTypeTagFromFile(fs::path filePath) { return std::string(); }

std::string CodecChooser::containerNameFromTag(const std::string& typeTag) {
    auto pos = typeTag.find_first_of("<>");
    if (pos == std::string::npos) {
        return std::string();
    }
    return typeTag.substr(0, pos);
}

std::string CodecChooser::codecNameFromTag(const std::string& typeTag) {
    auto pos = typeTag.find_last_of("<>");
    if (pos == std::string::npos || pos == typeTag.size() - 1) {
        return std::string();
    }
    return typeTag.substr(pos + 1);
}

} // namespace vk

} // namespace scin
