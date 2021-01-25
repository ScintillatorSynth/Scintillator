#include "base/AbstractSampler.hpp"

namespace scin { namespace base {

AbstractSampler::AbstractSampler(): m_key(0) {}

AbstractSampler::AbstractSampler(uint32_t key): m_key(key) {}

AbstractSampler::~AbstractSampler() {}

// 1 bit each for min and mag, bits 0-1 and 4-5.
void AbstractSampler::setMinFilterMode(AbstractSampler::FilterMode filterMode) {
    m_key = (m_key & 0xfffffff0) | filterMode;
}

AbstractSampler::FilterMode AbstractSampler::minFilterMode() const {
    return static_cast<FilterMode>(m_key & 0x0000000f);
}

void AbstractSampler::setMagFilterMode(AbstractSampler::FilterMode filterMode) {
    m_key = (m_key & 0xffffff0f) | (filterMode << 4);
}

AbstractSampler::FilterMode AbstractSampler::magFilterMode() const {
    return static_cast<FilterMode>((m_key & 0x000000f0) >> 4);
}

// 1 bit for anisotropic filtering, inverted so that 0 is the default (enabled), bit 8
void AbstractSampler::enableAnisotropicFiltering(bool enable) {
    if (enable) {
        m_key = m_key & 0xfffffeff;
    } else {
        m_key = m_key | 0x00000100;
    }
}

bool AbstractSampler::isAnisotropicFilteringEnabled() const { return (m_key & 0x00000100) == 0; }

// Two bits each for U and V addressing, bits 12 and 13 for U, 16 and 17 for V.
void AbstractSampler::setAddressModeU(AbstractSampler::AddressMode addressMode) {
    m_key = (m_key & 0xffff0fff) | addressMode;
}

AbstractSampler::AddressMode AbstractSampler::addressModeU() const {
    return static_cast<AddressMode>(m_key & 0x0000f000);
}

void AbstractSampler::setAddressModeV(AbstractSampler::AddressMode addressMode) {
    m_key = (m_key & 0xfff0ffff) | (addressMode << 4);
}

AbstractSampler::AddressMode AbstractSampler::addressModeV() const {
    return static_cast<AddressMode>((m_key & 0x000f0000) >> 4);
}

// Two bits for border color, bits 20 and 21
void AbstractSampler::setClampBorderColor(AbstractSampler::ClampBorderColor color) {
    m_key = (m_key & 0xff0fffff) | color;
}

AbstractSampler::ClampBorderColor AbstractSampler::clampBorderColor() const {
    return static_cast<ClampBorderColor>(m_key & 0x00f00000);
}

// 1 bit for using unnormalized coordinates, bit 24
void AbstractSampler::useUnnormalizedCoordinates(bool use) {
    if (use) {
        m_key = m_key | 0x01000000;
    } else {
        m_key = m_key & 0xf0ffffff;
    }
}

bool AbstractSampler::isUsingUnnormalizedCoordinates() const { return m_key & 0x01000000; }

} // namespace base

} // amespace scin
