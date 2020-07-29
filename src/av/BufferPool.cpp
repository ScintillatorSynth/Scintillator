#include "av/BufferPool.hpp"

#include "av/Buffer.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace av {

BufferPool::BufferPool(int width, int height): m_bufferPool(nullptr), m_width(width), m_height(height) {
    // Assuming 4-byte-per-pixel image formats we byte-align to 32 bytes or 8 pixels all image widths.
    m_stride = m_width;
    if (m_width % 8) {
        m_stride += (8 - (m_width % 8));
    }

    m_bufferPool = av_buffer_pool_init(m_stride * height * 4, nullptr);
    if (!m_bufferPool) {
        spdlog::error("failed to create BufferPool.");
    }

}

BufferPool::~BufferPool() { av_buffer_pool_uninit(&m_bufferPool); }

std::shared_ptr<Buffer> BufferPool::getBuffer() {
    AVBufferRef* bufferRef = av_buffer_pool_get(m_bufferPool);
    if (!bufferRef) {
        spdlog::error("BufferPool failed to create buffer.");
        return std::shared_ptr<Buffer>();
    }

    return std::shared_ptr<Buffer>(new Buffer(bufferRef, m_width, m_height, m_stride));
}

} // namespace av

} // namespace scin
