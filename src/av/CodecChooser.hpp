#ifndef SRC_AV_CODEC_CHOOSER_HPP_
#define SRC_AV_CODEC_CHOOSER_HPP_

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <string>
#include <unordered_map>
#include <vector>

namespace scin { namespace av {

// What we want are valid, supported pairs of input containers and supported decoder codecs, and output containers and
// their supported encoder codecs.

class CodecChooser {
public:
    CodecChooser();
    ~CodecChooser();

private:
};

} // namespace vk

} // namespace scin

#endif // SRC_AV_CODEC_CHOOSER_HPP_
