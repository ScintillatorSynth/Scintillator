#ifndef SRC_AV_IMAGE_DECODER_HPP_
#define SRC_AV_IMAGE_DECODER_HPP_

#include "av/AVIncludes.hpp"
#include "av/Decoder.hpp"

namespace scin { namespace av {

class ImageDecoder : public Decoder {
public:
    ImageDecoder();
    virtual ~ImageDecoder();

    /*! Open the media file at the supplied path and extract some basic metadata.
     *
     * \param filePath The path to the file to open.
     * \return True on success, false on error.
     */
    bool openFile(const fs::path& filePath) override;

    /*! Decode the image, convert to RGBA interleaved format, optionally scale, and copy to the bytes argument.
     *
     * \param bytes Where to extract the image to.
     * \param width The width of the desired extracted image.
     * \param height The height of the desired extracted image.
     * \return True on success, false on error.
     */
    bool extractImageTo(uint8_t* bytes, int width, int height);
};

} // namespace av
} // namespace scin


#endif // SRC_AV_IMAGE_DECODER_HPP_
