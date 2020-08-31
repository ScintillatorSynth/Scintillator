#include "av/Buffer.hpp"

namespace scin { namespace av {

Buffer::Buffer(AVBufferRef* bufferRef, int width, int height, int stride):
    m_bufferRef(bufferRef),
    m_width(width),
    m_height(height),
    m_stride(stride) {}

Buffer::~Buffer() { av_buffer_unref(&m_bufferRef); }

AVBufferRef* Buffer::addReference() { return av_buffer_ref(m_bufferRef); }

} // namespace av

} // namespace scin
