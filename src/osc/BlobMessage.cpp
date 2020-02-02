#include "BlobMessage.hpp"

#include <cstring>

namespace scin { namespace osc {

BlobMessage::BlobMessage(): m_size(0) {}

BlobMessage::~BlobMessage() {
    if (m_message) {
        lo_message_free(m_message);
        m_message = nullptr;
    }
}

bool BlobMessage::extract(lo_blob blob) {
    m_size = lo_blob_datasize(blob);
    if (m_size == 0) {
        return false;
    }
    m_bytes.reset(new uint8_t[m_size]);
    std::memcpy(m_bytes.get(), lo_blob_dataptr(blob), m_size);
    m_message = lo_message_deserialise(m_bytes.get(), m_size, nullptr);
    if (!m_message) {
        return false;
    }
    return true;
}

} // namespace osc
} // namespace scin
