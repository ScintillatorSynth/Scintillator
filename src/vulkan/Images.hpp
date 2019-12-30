#ifndef SRC_VULKAN_IMAGES_HPP_
#define SRC_VULKAN_IMAGES_HPP_

namespace scin {

namespace vk {

/*! Wraps one or more Vulkan VkImage objects, and associated metadata.
 */
class Images {
public:
    Images();
    ~Images();

    // call vkGetSwapchainImagesKHR
    void getFromSwapchain(Swapchain* swapchain);

    size_t count() const { return m_images.size(); }

private:
    std::vector<VkImage> m_images;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_IMAGES_HPP_
