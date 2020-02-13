#ifndef SRC_AV_IMAGE_DECODER_HPP_
#define SRC_AV_IMAGE_DECODER_HPP_

#include "av/AVIncludes.hpp"
#include "av/Decoder.hpp"

namespace scin { namespace av {

class ImageDecoder : public Decoder {
public:
    ImageDecoder();
    virtual ~ImageDecoder();

    bool openFile(const fs::path& filePath) override;
};

} // namespace av
} // namespace scin


#endif // SRC_AV_IMAGE_DECODER_HPP_
