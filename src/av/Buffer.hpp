#ifndef SRC_AV_BUFFER_HPP_
#define SRC_AV_BUFFER_HPP_

#include "av/AVIncludes.hpp"

namespace scin { namespace av {

/*! Wraps an internally allocated libavcodec buffer, and can be associated with Frames for shared encode of rendered
 * data.
 */
class Buffer {
public:
    /*! Typically Buffers are allocated by BufferPool, which uses this constructor.
     */
    Buffer(AVBufferRef* bufferRef);
    ~Buffer();

    AVBufferRef* addReference();

    uint8_t* data() { return m_bufferRef->data; }
    int size() { return m_bufferRef->size; }

private:
    AVBufferRef* m_bufferRef;
};

} // namespace av

} // namespace scin

#endif // SRC_AV_BUFFER_HPP_
