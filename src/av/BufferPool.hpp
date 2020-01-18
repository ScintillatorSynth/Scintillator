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

private:
    AVBufferPool* m_bufferPool;
};

} // namespace av

} // namespace scin

#endif // SRC_AV_BUFFER_POOL_HPP_
