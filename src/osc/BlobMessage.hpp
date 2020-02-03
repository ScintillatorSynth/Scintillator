#ifndef SRC_OSC_BLOB_MESSAGE_HPP_
#define SRC_OSC_BLOB_MESSAGE_HPP_

#include "lo/lo.h"

#include <memory>

namespace scin { namespace osc {
/*! Can extract a copy of a message from a OSC blob lo_blob data structure.
 */
class BlobMessage {
public:
    BlobMessage();
    ~BlobMessage();

    bool extract(lo_blob blob);

    lo_message message() { return m_message; }
    const char* path() const { return reinterpret_cast<const char*>(m_bytes.get()); }

private:
    std::unique_ptr<uint8_t[]> m_bytes;
    uint32_t m_size;
    lo_message m_message;
};

} // namespace osc
} // namespace scin

#endif // SRC_OSC_BLOB_MESSAGE_HPP_
