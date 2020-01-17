#ifndef SRC_AV_ENCODER_HPP_
#define SRC_AV_ENCODER_HPP_

#include "core/FileSystem.hpp"

#include <string>

namespace scin { namespace av {

/*! Abstract base class for encoders? Maybe ImageEncoder, VideoEncoder, AudioEncoder? Or Video also handles audio?
 */
class Encoder : public Context {
public:
    Encoder();
    virtual ~Encoder();

    /*! Creates a file and prepares for encoding.
     *
     * \param filePath The path of the file to open, including extension, which provides an important hint to the
     *        encoder library as to the type of media file to encode.
     * \param mimeType Optional string argument, either empty or a mime type, providing a further hint to the encoder
     *        about the type of container and codec to use to encode this file.
     * \return true on succcess, false on failure.
     */
    bool createFile(const fs::path& filePath, const std::string& mimeType) = 0;

protected:
    AVOutputFormat* m_outputFormat;
};

} // namespace av

} // namespace scin

#endif // SRC_AV_ENCODER_HPP_
