#ifndef SRC_AV_PACKET_HPP_
#define SRC_AV_PACKET_HPP_

#include "av/AVIncludes.hpp"

namespace scin { namespace av {

/*! RAII-style AVPacket wrapper.
 */
class Packet {
public:
    Packet();
    ~Packet();

    bool create();
    void destroy();

    AVPacket* get() { return m_packet; }

private:
    AVPacket* m_packet;
};

} // namespace av

} // namespace scin

#endif // SRC_AV_PACKET_HPP_
