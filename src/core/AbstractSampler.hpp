#ifndef SRC_CORE_ABSTRACT_SAMPLER_HPP_
#define SRC_CORE_ABSTRACT_SAMPLER_HPP_

#include <cstdint>

namespace scin { namespace core {

/*! Represents a set of configuration parameters for a Vulkan 2D image sampler. Packs the configuration into a single
 * unsigned int value to allow coalescing of Samplers across Vulkan, as they are a resource available in limited
 * quantities.
 */
class AbstractSampler {
public:
    AbstractSampler();
    explicit AbstractSampler(uint32_t key);
    ~AbstractSampler();
    AbstractSampler(const AbstractSampler&) = default;
    AbstractSampler& operator=(const AbstractSampler&) = default;

    enum FilterMode : uint32_t { kLinear = 0, kNearest = 1 };
    void setMinFilterMode(FilterMode filterMode);
    FilterMode minFilterMode() const;
    void setMagFilterMode(FilterMode filterMode);
    FilterMode magFilterMode() const;

    void enableAnisotropicFiltering(bool enable);
    bool isAnisotropicFilteringEnabled() const;

    enum AddressMode : uint32_t {
        kClampToBorder = 0,
        kClampToEdge = 0x1000,
        kRepeat = 0x2000,
        kMirroredRepeat = 0x3000
    };
    void setAddressModeU(AddressMode addressMode);
    AddressMode addressModeU() const;
    void setAddressModeV(AddressMode addressMode);
    AddressMode addressModeV() const;

    enum ClampBorderColor : uint32_t { kTransparentBlack = 0, kBlack = 0x100000, kWhite = 0x200000 };
    void setClampBorderColor(ClampBorderColor color);
    ClampBorderColor clampBorderColor() const;

    uint32_t key() const { return m_key; }

private:
    uint32_t m_key;
};

} // namespace core

} // namespace scin

#endif // SRC_CORE_ABSTRACT_SAMPLER_HPP_
