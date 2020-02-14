#include "av/ImageDecoder.hpp"

#include "av/Frame.hpp"
#include "av/Packet.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace av {

ImageDecoder::ImageDecoder() {}

ImageDecoder::~ImageDecoder() {}

bool ImageDecoder::openFile(const fs::path& filePath) {
    // Open file and read media header.
    if (avformat_open_input(&m_formatContext, filePath.string().data(), nullptr, nullptr) != 0) {
        spdlog::error("ImageDecoder failed to open file {}", filePath.string());
        return false;
    }
    if (avformat_find_stream_info(m_formatContext, nullptr) != 0) {
        spdlog::error("ImageDecoder failed to read stream info for file {}", filePath.string());
        return false;
    }

    int streamIndex = av_find_best_stream(m_formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &m_codec, 0);
    if (streamIndex < 0) {
        spdlog::error("ImageDecoder unable to find decodeable image stream in file {}", filePath.string());
        return false;
    }

    m_codecContext = avcodec_alloc_context3(m_codec);
    if (!m_codecContext) {
        spdlog::error("ImageDecoder failed to allocate the codec context for file {}", filePath.string());
        return false;
    }

    if (avcodec_parameters_to_context(m_codecContext, m_formatContext->streams[streamIndex]->codecpar) < 0) {
        spdlog::error("ImageDecoder failed to copy codec parameters to codec context for file {}", filePath.string());
        return false;
    }

    if (avcodec_open2(m_codecContext, m_codec, nullptr) < 0) {
        spdlog::error("ImageDecoder failed to open decoder for file {}", filePath.string());
        return false;
    }

    m_width = m_codecContext->width;
    m_height = m_codecContext->height;

    return true;
}

bool ImageDecoder::extractImageTo(uint8_t* bytes, int width, int height) {
    Packet packet;
    if (av_read_frame(m_formatContext, packet.get()) < 0) {
        spdlog::error("ImageDecoder unable to read packet.");
        return false;
    }

    if (avcodec_send_packet(m_codecContext, packet.get()) < 0) {
        spdlog::error("ImageDecoder unable to decode packet.");
        return false;
    }

    Frame frame;
    if (!frame.createEmpty()) {
        spdlog::error("ImageDecoder failed to create Frame.");
        return false;
    }

    if (avcodec_receive_frame(m_codecContext, frame.get()) < 0) {
        spdlog::error("ImageDecoder failed to decode image.");
        return false;
    }

    SwsContext* scaleContext = sws_getContext(m_width, m_height, m_codecContext->pix_fmt, width, height,
                                              AV_PIX_FMT_RGBA, SWS_BICUBIC, nullptr, nullptr, nullptr);
    uint8_t* outputPointers[] = { bytes };
    int outputSizes[] = { width * height * 4 };
    bool result =
        (sws_scale(scaleContext, frame.get()->data, frame.get()->linesize, 0, m_height, outputPointers, outputSizes)
         <= 0);
    sws_freeContext(scaleContext);
    return result;
}

} // namespace av
} // namespace scin
