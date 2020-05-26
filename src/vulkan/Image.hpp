#ifndef SRC_VULKAN_IMAGE_HPP_
#define SRC_VULKAN_IMAGE_HPP_

#include "vulkan/Vulkan.hpp"

#include "vk_mem_alloc.h"

#include <memory>

namespace scin { namespace vk {

class Device;

/*! Abstract base class describing a Vulkan Image.
 */
class Image {
public:
    explicit Image(std::shared_ptr<Device> device);
    virtual ~Image();
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    /*! Create a VkImageView associated with this image, if required. The view will be accessible via the view()
     * function.
     *
     * \return true on success, false on failure.
     */
    bool createView();

    VkImage get() const { return m_image; }
    VkFormat format() const { return m_format; }
    VkExtent2D extent() const { return m_extent; }
    uint32_t width() const { return m_extent.width; }
    uint32_t height() const { return m_extent.height; }
    VkImageView view() const { return m_imageView; }

protected:
    Image(std::shared_ptr<Device> device, VkImage image, VkFormat format, VkExtent2D extent);

    std::shared_ptr<Device> m_device;
    VkImage m_image;
    VkFormat m_format;
    VkExtent2D m_extent;
    VkImageView m_imageView;
};

/*! Non-owning wrapper around a Swapchain Image.
 */
class SwapchainImage : public Image {
public:
    SwapchainImage(std::shared_ptr<Device> device, VkImage image, VkFormat format, VkExtent2D extent);
    virtual ~SwapchainImage();
    SwapchainImage(const SwapchainImage&) = delete;
    SwapchainImage& operator=(const SwapchainImage&) = delete;
};

/*! Base class for images allocated by the process, meaning they will need to be reclaimed.
 */
class AllocatedImage : public Image {
public:
    explicit AllocatedImage(std::shared_ptr<Device> device, VkFormat format);
    virtual ~AllocatedImage();
    AllocatedImage(const AllocatedImage&) = delete;
    AllocatedImage& operator=(const AllocatedImage&) = delete;

    virtual bool create(uint32_t width, uint32_t height) = 0;
    void destroy();

    int size() const { return m_info.size; }

protected:
    VmaAllocation m_allocation;
    VmaAllocationInfo m_info;
};

/*! Represents image memory that is only accessible by the GPU device. Can be faster for GPU access than host-coherent
 * memory in some architectures.
 */
class DeviceImage : public AllocatedImage {
public:
    explicit DeviceImage(std::shared_ptr<Device> device, VkFormat format);
    virtual ~DeviceImage();
    DeviceImage(const DeviceImage&) = delete;
    DeviceImage& operator=(const DeviceImage&) = delete;

    bool create(uint32_t width, uint32_t height) override;

protected:
    bool createDeviceImage(uint32_t width, uint32_t height, bool isFramebuffer);
};

/*! GPU-accessible memory that can also serve as a render target.
 */
class FramebufferImage : public DeviceImage {
public:
    explicit FramebufferImage(std::shared_ptr<Device> device);
    virtual ~FramebufferImage();
    FramebufferImage(const FramebufferImage&) = delete;
    FramebufferImage& operator=(const FramebufferImage&) = delete;

    bool create(uint32_t width, uint32_t height) override;
};

/*! Host-accessible GPU memory that allows readback into a buffer suitable for CPU-side media encoding.
 */
class HostImage : public AllocatedImage {
public:
    explicit HostImage(std::shared_ptr<Device> device);
    virtual ~HostImage();
    HostImage(const HostImage&) = delete;
    HostImage& operator=(const HostImage&) = delete;

    bool create(uint32_t width, uint32_t height) override;

    void* mappedAddress() { return m_info.pMappedData; }

protected:
};


} // namespace vk
} // namespace scin

#endif // SRC_VULKAN_IMAGE_HPP_
