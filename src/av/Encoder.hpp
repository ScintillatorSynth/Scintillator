#ifndef SRC_AV_ENCODER_HPP_
#define SRC_AV_ENCODER_HPP_

#include "av/AVIncludes.hpp"
#include "core/FileSystem.hpp"

#include <functional>
#include <memory>
#include <string>

namespace scin { namespace av {

class Buffer;

/*! Abstract base class for encoders? Maybe ImageEncoder, VideoEncoder, AudioEncoder? Or Video also handles audio?
 */
class Encoder {
public:
    Encoder(int width, int height, std::function<void(bool)> completion);
    virtual ~Encoder();

    /*! Creates a file and prepares for encoding.
     *
     * \param filePath The path of the file to open, including extension, which provides an important hint to the
     *        encoder library as to the type of media file to encode.
     * \param mimeType Optional string argument, either empty or a mime type, providing a further hint to the encoder
     *        about the type of container and codec to use to encode this file.
     * \return true on succcess, false on failure.
     */
    virtual bool createFile(const fs::path& filePath, const std::string& mimeType) = 0;

    typedef std::function<void(std::shared_ptr<scin::av::Buffer>)> SendBuffer;

    /*! Provides a function to call when the next buffer has been rendered.
     *
     * \param frameTime The overall render time associated with the frame that will be provided.
     * \param frameNumber The counting number of the frame.
     * \param callbackOut On return will be populated with a callback function to be called when the Buffer is ready.
     * \return True if this encoder should continue to be called for this and subsequent frames, false otherwise.
     */
    virtual bool queueEncode(double frameTime, size_t frameNumber, SendBuffer& callbackOut) = 0;

protected:
    void finishEncode(bool valid);

    int m_width;
    int m_height;
    std::function<void(bool)> m_completion;
    AVOutputFormat* m_outputFormat;
    AVCodec* m_codec;
    AVCodecContext* m_codecContext;
    AVFormatContext* m_formatContext;
};

} // namespace av

} // namespace scin

#endif // SRC_AV_ENCODER_HPP_
