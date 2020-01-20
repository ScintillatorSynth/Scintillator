#include "av/Packet.hpp"

namespace scin { namespace av {

Packet::Packet(): m_packet(nullptr) {}

Packet::~Packet() { destroy(); }

bool Packet::create() {
    m_packet = av_packet_alloc();
    return m_packet != nullptr;
}

void Packet::destroy() {
    if (m_packet != nullptr) {
        av_packet_free(&m_packet);
    }
}

} // namespace av

} // namespace scin
