#ifndef SRC_VULKAN_IMAGES_HPP_
#define SRC_VULKAN_IMAGES_HPP_

#include "vulkan/Vulkan.hpp"

#include <memory>
#include <vector>

namespace scin {

namespace vk {

class Device;
class Swapchain;

/*! Wraps one or more Vulkan VkImage objects, and associated metadata.
 */
class Images {
public:
    Images(std::shared_ptr<Device> device);
    ~Images();

    /*! Extracts the set of images from the swapchain.
     *
     * \param swapchain The Swapchain class to extract the Images from.
     * \param imageCount The expected number of images. Vulkan may actually allocate more images than this.
     * \return The actual number of images allocated.
     */
    uint32_t getFromSwapchain(Swapchain* swapchain, uint32_t imageCount);

    size_t count() const { return m_images.size(); }
    const std::vector<VkImage>& get() const { return m_images; }
    VkFormat format() const { return m_format; }
    VkExtent2D extent() const { return m_extent; }
    uint32_t width() const { return m_extent.width; }
    uint32_t height() const { return m_extent.height; }

private:
    std::shared_ptr<Device> m_device;
    std::vector<VkImage> m_images;
    VkFormat m_format;
    VkExtent2D m_extent;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_IMAGES_HPP_
