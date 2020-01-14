#ifndef SRC_AV_CODEC_CHOOSER_HPP_
#define SRC_AV_CODEC_CHOOSER_HPP_

#include "core/FileSystem.hpp"

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

    std::string getTypeTag(const std::string& typeTag, const std::string& mimeType);
    std::string getTypeTagFromFile(fs::path filePath);

    std::string containerNameFromTag(const std::string& typeTag);
    std::string codecNameFromTag(const std::string& typeTag);

private:
};

} // namespace vk

} // namespace scin

#endif // SRC_AV_CODEC_CHOOSER_HPP_
