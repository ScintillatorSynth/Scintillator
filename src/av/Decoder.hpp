#ifndef SRC_AV_DECODER_HPP_
#define SRC_AV_DECODER_HPP_

#include "av/AVIncludes.hpp"
#include "core/FileSystem.hpp"

namespace scin { namespace av {

/*! Abstract base class for media decoding.
 */
class Decoder {
public:
    Decoder();
    virtual ~Decoder();

    /*! Opens a media file, reads some basic information, and determines if a decode may be possible. Needs to be called
     * before width() and height() functions will return valid values.
     *
     * \param filePath The path to the media file to open.
     * \return True if the file opened successfully, false if not.
     */
    virtual bool openFile(const fs::path& filePath) = 0;

    int width() const { return m_width; }
    int height() const { return m_height; }

protected:
    int m_width;
    int m_height;

    AVCodecContext* m_codecContext;
    AVFormatContext* m_formatContext;
    AVCodec* m_codec;
};

} // namespace av
} // namespace scin

#endif // SRC_AV_DECODER_HPP_
