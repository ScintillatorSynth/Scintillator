#ifndef SRC_AV_CONTEXT_HPP_
#define SRC_AV_CONTEXT_HPP_

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <deque>
#include <memory>
#include <mutex>

namespace scin { namespace av {

class Frame {
public:
    Frame();
    ~Frame();
};


/*! Abstract base class for all media encoding and decoding. Wraps an ffmpeg AVCodecContext, which is a codec-specific
 *  state.
 */
class Context {
public:
    Context();
    virtual ~Context();

    // So we have an encoders and decoders per codec, created with avcodec_find_encoder or decoder.
    // Then we allocate a context with avcodec_alloc_context3, which needs to be freed.
    // We set some options like size, etc.
    // Then an (expensive?) call to avcodec_open2, not thread safe. Maybe some kind of global mutex?
    // At this point I think things can be resued, so perhaps there's a class heirachy like
    // Context -> EncodeContext or DecodeContext -> PNGEncoder or some such.

    // For writing:
    // We open a file with avio_open, which takes a context. Then avformat_init_output, followed by
    // avformat_write_header, then av_write_frame (until finished), then av_write_trailer. then avio_close.

    // We make an avframe and allocate it with av_frame_alloc, it needs some metadata like width and height and format
    // allocate buffer memory with av_frame_get_buffer (q: are frames reusable? they are)
    // we make it writeable, send it to the encoder (via the context), receive one or more packets and write them,
    // then finished.

    virtual bool create() = 0;


protected:
    bool createCodecContext(AVCodecID codecID);

    AVCodecContext* m_context;
};

/*! Abstract base class for encoders.
 */
class Encoder : public Context {
public:
    Encoder();
    virtual ~Encoder();

    std::shared_ptr<Frame> getEmptyFrame();

protected:
    std::mutex m_emptyFramesMutex;
    std::deque<std::shared_ptr<Frame>> m_emptyFrames;
};

class PNGEncoder : public Encoder {
public:
    PNGEncoder();
    virtual ~PNGEncoder();

    bool create() override;
};


} // namespace av

} // namespace scin

#endif // SRC_AV_CONTEXT_HPP_
