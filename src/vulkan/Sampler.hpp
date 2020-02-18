#ifndef SRC_VULKAN_SAMPLER_HPP_
#define SRC_VULKAN_SAMPLER_HPP_

#include "vulkan/Vulkan.hpp"

#include <memory>

namespace scin { namespace vk {

class Device;
class HostImage;

/*! Represents all the required Vulkan state to use an image as a texture.
 */
class Sampler {
public:
    Sampler(std::shared_ptr<Device> device, std::shared_ptr<HostImage> image);
    ~Sampler();

    bool create();
    void destroy();

private:
    std::shared_ptr<Device> m_device;
    std::shared_ptr<HostImage> m_image;

    VkImageView m_imageView;
    VkSampler m_sampler;
};

} // namespace vk
} // namespace scin

#endif // SRC_VULKAN_SAMPLER_HPP_
