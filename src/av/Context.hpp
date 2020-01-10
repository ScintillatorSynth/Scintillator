#ifndef SRC_AV_CONTEXT_HPP_
#define SRC_AV_CONTEXT_HPP_

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace scin { namespace av {

/*! Wraps an ffmpeg AVCodecContext, which is a codec-specific decode or encode state.
 */
class Context {
public:
    enum Codec { kPNG, kJPEG };

    Context(Codec codec);
    ~Context();

    bool create();

private:
    Codec m_codec;
};

} // namespace av

} // namespace scin

#endif // SRC_AV_CONTEXT_HPP_
