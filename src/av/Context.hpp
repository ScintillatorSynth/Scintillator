#ifndef SRC_AV_CONTEXT_HPP_
#define SRC_AV_CONTEXT_HPP_

#include "av/AVIncludes.hpp"

namespace scin { namespace av {

/*! Abstract base class for media encoding and decoding.
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

} // namespace av

} // namespace scin

#endif // SRC_AV_CONTEXT_HPP_
