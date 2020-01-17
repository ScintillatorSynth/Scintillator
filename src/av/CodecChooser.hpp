#ifndef SRC_AV_CODEC_CHOOSER_HPP_
#define SRC_AV_CODEC_CHOOSER_HPP_

#include "core/FileSystem.hpp"

#include "av/AVIncludes.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace scin { namespace av {

/*! TODO: this whole class seems kind of nuts. Solving a problem we may not have. Particularly by breaking the encoders
 * at least into image/video/audio subclasses, it might be pretty obvious to avcodec what format to choose.
 */
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
