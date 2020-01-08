#ifndef SRC_VULKAN_IMAGE_SET_HPP_
#define SRC_VULKAN_IMAGE_SET_HPP_

#include "vulkan/Vulkan.hpp"

#include "vk_mem_alloc.h"

#include <memory>
#include <vector>

namespace scin { namespace vk {

class Device;
class Swapchain;

/*! Wraps one or more Vulkan VkImage objects, and associated metadata.
 */
class ImageSet {
public:
    ImageSet(std::shared_ptr<Device> device);
    ~ImageSet();

    /*! Wraps the set of images from the swapchain.
     *
     * \note The ImageSet does not own these Images and will not delete them upon destruction.
     *
     * \param swapchain The Swapchain class to extract the images from.
     * \param imageCount The expected number of images. Vulkan may actually allocate more images than this.
     * \return The actual number of images allocated, 0 on error.
     */
    uint32_t getFromSwapchain(Swapchain* swapchain, uint32_t imageCount);

    /*! Make a set of images backed by allocated memory in a format accessible from the host CPU.
     *
     * \note The ImageSet does own these Images and will delete them upon destruction.
     *
     * \param width The width of each image in the set.
     * \param height The height of each image in the set.
     * \param numberOfImages The number of images to create for the set.
     * \return true on success, false on failure.
     */
    bool createHostTransferTarget(uint32_t width, uint32_t height, size_t numberOfImages);

    void destroy();

    size_t count() const { return m_images.size(); }
    const std::vector<VkImage>& get() const { return m_images; }
    VkFormat format() const { return m_format; }
    VkExtent2D extent() const { return m_extent; }
    uint32_t width() const { return m_extent.width; }
    uint32_t height() const { return m_extent.height; }

private:
    std::shared_ptr<Device> m_device;
    std::vector<VkImage> m_images;
    std::vector<VmaAllocation> m_allocations;
    VkFormat m_format;
    VkExtent2D m_extent;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_IMAGE_SET_HPP_
