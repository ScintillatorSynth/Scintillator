#ifndef SRC_AV_IMAGE_ENCODER_HPP_
#define SRC_AV_IMAGE_ENCODER_HPP_

#include "av/AVIncludes.hpp"
#include "av/Encoder.hpp"

namespace scin { namespace av {

class ImageEncoder : public Encoder {
public:
    ImageEncoder();
    virtual ~ImageEncoder();

    bool createFile(const fs::path& filePath, const std::string& mimeType) override;

private:

};

} // namespace av

} // namespace scin

#endif // SRC_AV_IMAGE_ENCODER_HPP_
