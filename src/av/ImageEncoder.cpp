#include "av/ImageEncoder.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace av {

ImageEncoder::ImageEncoder(int width, int height): Encoder(width, height) {
}

ImageEncoder::~ImageEncoder() {
    if (m_outputContext) {
        avformat_free_context(m_outputContext);
        m_outputContext = nullptr;
    }
}

// TODO: completion callback on failure?
bool ImageEncoder::createFile(const fs::path& filePath, const std::string& mimeType) {
    const char* mimeString = mimeType.size() ? mimeType.data() : nullptr;
    m_outputFormat = av_guess_format(nullptr, filePath.string().data(), mimeString);
    if (!m_outputFormat) {
        spdlog::error("Encoder unable to guess output format for file {}, mime type {}", filePath.string(), mimeType);
        return false;
    }
    AVCodecID codecID = av_guess_codec(m_outputFormat, nullptr, filePath.string().data(), mimeString,
            AVMEDIA_TYPE_IMAGE);
    m_codec = avcodec_find_encoder(codecID);
    if (!m_codec) {
        splog::error("Encoder unable to guess code for file {}, mime type {}, output format name {}", filePath.string(),
                mimeType, m_outputFormat->name);
        return false;
    }

    spdlog::info("Encoder chose output format name {}, codec name {} for file {}, mime type {}", m_outputFormat->name,
            m_codec->name, filePath.string(), mimeType);

    if (avformat_alloc_output_context2(&m_outputContext, m_outputFormat, nullptr, filePath.string()) < 0) {
        spdlog::error("Encoder failed to allocate output context for file {}", filePath.string());
        return false;
    }

    m_codecContext = avcodec_alloc_context3(m_codec);
    if (!m_codecContext) {
        spdlog::error("Encoder unable to allocate codec context for codec {}", m_codec->name);
        return false;
    }

    if (avio_open(&m_outputContext->pb, filePath.string().data(), AVIO_FLAG_WRITE) < 0) {
        spdlog::error("Encoder failed to open file {}", filePath.string());
        return false;
    }

    // Need some metadata about the encode before writing any header information.
    m_codecContext->width = m_width;
    m_codecContext->height = m_height;
    m_codecContext->coded_width = m_width;
    m_codecContext->coded_height = m_height;
    m_codecContext->pix_fmt = AV_PIX_FMT_RGBA;

    m_outputContext->width = m_width;
    m_outputContext->height = m_height;

    if (avformat_write_header(m_outputContext, nullptr) < 0) {
        spdlog::error("Encoder failed to write header to file {}", filePath.string());
        return false;
    }
    return true;
}

bool ImageEncoder::queueEncode(double frameTime, size_t frameNumber, SendBuffer& callbackOut) {
    spdlog::info("ImageEncoder queuing buffer for render at time {}, frame Number {}", frameTime, frameNumber);
    callbackOut = SendBuffer([this](std::shared_ptr<const Buffer> buffer) {
        Frame frame(buffer);
        // For image encoding we're going to send exactly one frame, so this should work unless there's an encoding
        // error, meaning we treat all error results as fatal encoding errors.
        // TODO: where does m_codecContext come from? We have one in Context.hpp but never initialize it.
        int ret = avcodec_send_frame(m_codecContext, frame.get());
        if (ret < 0) {
            spdlog::error("ImageEncoder failed encoding image frame.");
            finishEncode(false);
            return;
        }
        AVPacket packet;
        ret = avcodec_receive_packet(m_codecContext, &packet);
        while (ret == 0) {
            if (av_write_frame(m_outputContext, &packet) < 0) {
                spdlog::error("ImageEncoder failed writing frame data.");
                finsihEncode(false);
                return;
            }
            ret = avcodec_receive_packet(m_codecContext, &packet);
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
