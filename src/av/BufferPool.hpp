#ifndef SRC_AV_BUFFER_POOL_HPP_
#define SRC_AV_BUFFER_POOL_HPP_

#include "av/AVIncludes.hpp"

#include <memory>

namespace scin { namespace av {

class Buffer;

class BufferPool {
public:
    BufferPool(int width, int height);

    /*! Does not require that all of the allocated Buffers need to have been released first.
     */
    ~BufferPool();

    std::shared_ptr<Buffer> getBuffer();

    /*! Some codecs have a pixel row alignment requirement for rows of pixels. BufferPool assumes all codecs will, and
     * so may add to the width of each row to allow for that alignment. This byte pixel width is the stride and is
     * accessed here.
     */
    int stride() const { return m_stride; }

private:
    AVBufferPool* m_bufferPool;
    int m_width;
    int m_height;
    int m_stride;
};

} // namespace av

} // namespace scin

#endif // SRC_AV_BUFFER_POOL_HPP_
