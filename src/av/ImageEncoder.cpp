#include "av/ImageEncoder.hpp"

#include "av/Frame.hpp"
#include "av/Packet.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace av {

ImageEncoder::ImageEncoder(int width, int height, std::function<void(bool)> completion):
    Encoder(width, height, completion) {}

ImageEncoder::~ImageEncoder() {
    if (m_formatContext) {
        avformat_free_context(m_formatContext);
        m_formatContext = nullptr;
    }
}

// TODO: completion callback on failure?
bool ImageEncoder::createFile(const fs::path& filePath, const std::string& mimeType) {
    const char* mimeString = mimeType.size() ? mimeType.data() : nullptr;
    m_outputFormat = av_guess_format("image2", filePath.string().data(), mimeString);
    if (!m_outputFormat) {
        spdlog::error("Encoder unable to guess output format for file '{}', mime type '{}'", filePath.string(),
                      mimeType);
        return false;
    }
    AVCodecID codecID =
        av_guess_codec(m_outputFormat, nullptr, filePath.string().data(), mimeString, AVMEDIA_TYPE_VIDEO);
    m_codec = avcodec_find_encoder(codecID);
    if (!m_codec) {
        spdlog::error("Encoder unable to guess codec for file '{}', mime type '{}', output format name '{}'",
                      filePath.string(), mimeType, m_outputFormat->name);
        return false;
    }

    spdlog::info("Encoder chose output format name '{}', codec name '{}' for file '{}', mime type '{}'",
                 m_outputFormat->name, m_codec->name, filePath.string(), mimeType);

    if (avformat_alloc_output_context2(&m_formatContext, m_outputFormat, nullptr, filePath.string().data()) < 0) {
        spdlog::error("Encoder failed to allocate output context for file {}", filePath.string());
        return false;
    }

    m_codecContext = avcodec_alloc_context3(m_codec);
    if (!m_codecContext) {
        spdlog::error("Encoder unable to allocate codec context for codec '{}'", m_codec->name);
        return false;
    }

    // Need some metadata about the encode before writing any header information.
    m_codecContext->width = m_width;
    m_codecContext->height = m_height;
    m_codecContext->coded_width = m_width;
    m_codecContext->coded_height = m_height;
    m_codecContext->pix_fmt = AV_PIX_FMT_RGBA;
    // Set an arbitrary frame rate of 1 for images, or some encoders complain.
    m_codecContext->time_base = AVRational { 1, 1 };

    if (avcodec_open2(m_codecContext, m_codec, nullptr) < 0) {
        spdlog::error("Encoder failed to open codec '{}'", m_codec->name);
        return false;
    }

    // Add a video stream for output image to the output context.
    AVStream* stream = avformat_new_stream(m_formatContext, nullptr);
    avcodec_parameters_from_context(stream->codecpar, m_codecContext);

    if (avio_open(&m_formatContext->pb, filePath.string().data(), AVIO_FLAG_WRITE) < 0) {
        spdlog::error("Encoder failed to open file '{}'", filePath.string());
        return false;
    }

    if (avformat_write_header(m_formatContext, nullptr) < 0) {
        spdlog::error("Encoder failed to write header to file '{}'", filePath.string());
        return false;
    }
    return true;
}

// TODO: there really has to be a less ugly way of ensuring the callback gets called through all paths here.
bool ImageEncoder::queueEncode(double frameTime, size_t frameNumber, SendBuffer& callbackOut) {
    spdlog::debug("ImageEncoder queuing buffer for render at time {}, frame Number {}", frameTime, frameNumber);

    callbackOut = SendBuffer([this](std::shared_ptr<Buffer> buffer) {
        spdlog::debug("ImageEncoder got callback to encode frame.");
        Frame frame(buffer);
        if (!frame.create()) {
            spdlog::error("ImageEncoder failed creating frame.");
            finishEncode(false);
            return;
        }

        // For image encoding we're going to send exactly one frame, so this should work unless there's an encoding
        // error, meaning we treat all error results as fatal encoding errors.
        int ret = avcodec_send_frame(m_codecContext, frame.get());
        if (ret < 0) {
            spdlog::error("ImageEncoder failed encoding image frame.");
            finishEncode(false);
            return;
        }
        spdlog::trace("ImageEncoder through avcodec_send_frame.");

        Packet packet;
        if (!packet.create()) {
            spdlog::error("ImageEncoder failed to allocate packet.");
            finishEncode(false);
            return;
        }

        ret = avcodec_receive_packet(m_codecContext, packet.get());
        while (ret >= 0) {
            spdlog::trace("ImageEncoder through avcodec_receive_packet.");
            if (av_write_frame(m_formatContext, packet.get()) < 0) {
                spdlog::error("ImageEncoder failed writing frame data.");
                finishEncode(false);
                return;
            }
            spdlog::trace("ImageEncoder through av_write_frame.");
            ret = avcodec_receive_packet(m_codecContext, packet.get());
        }

        if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
            spdlog::error("ImageEncoder failed encoding image data.");
            finishEncode(false);
            return;
        }

        finishEncode(true);
    });

    // As this is an image it should only ever be called once.
    return false;
}


} // namespace av

} // namespace scin
