#ifndef SRC_VULKAN_SAMPLER_HPP_
#define SRC_VULKAN_SAMPLER_HPP_

#include "core/AbstractSampler.hpp"

#include "vulkan/Vulkan.hpp"

#include <memory>

namespace scin { namespace vk {

class Device;
class HostImage;

/*! Wraps a Vulkan VkSampler object, and is configurable using an AbstractSampler.
 */
class Sampler {
public:
    Sampler(std::shared_ptr<Device> device, const core::AbstractSampler& abstractSampler);
    ~Sampler();

    bool create();
    void destroy();

    const core::AbstractSampler& abstractSampler() const { return m_abstractSampler; }

private:
    std::shared_ptr<Device> m_device;
    core::AbstractSampler m_abstractSampler;

    VkSampler m_sampler;
};

} // namespace vk
} // namespace scin

#endif // SRC_VULKAN_SAMPLER_HPP_
