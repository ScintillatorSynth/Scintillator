#include "av/BufferPool.hpp"

#include "av/Buffer.hpp"

#include "spdlog/spdlog.h"

namespace scin {

namespace av {

BufferPool::BufferPool(int width, int height): m_bufferPool(nullptr) {
    m_bufferPool = av_buffer_pool_init(width * height * 4, nullptr);
    if (!m_bufferPool) {
        spdlog::error("failed to create BufferPool.");
    }
}

BufferPool::~BufferPool() {
    av_buffer_pool_uninit(&m_bufferPool);
}

std::shared_ptr<Buffer> BufferPool::getBuffer() {
    AVBufferRef* bufferRef = av_buffer_pool_get(m_bufferPool);
    if (!bufferRef) {
        spdlog::error("BufferPool failed to create buffer.");
        return std::shared_ptr<Buffer>();
    }

    return std::shared_ptr<Buffer>(new Buffer(bufferRef));
}

} // namespace av

} // namespace scin
