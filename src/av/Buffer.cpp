#include "av/Buffer.hpp"

namespace scin { namespace av {

Buffer::Buffer(AVBufferRef* bufferRef): m_bufferRef(bufferRef) {}

Buffer::~Buffer() { av_buffer_unref(&m_bufferRef); }

} // namespace av

} // namespace scin