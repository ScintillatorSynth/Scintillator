#ifndef SRC_VULKAN_SWAP_CHAIN_H_
#define SRC_VULKAN_SWAP_CHAIN_H_

#include "vulkan/Vulkan.hpp"

#include <memory>
#include <vector>

namespace scin {

namespace vk {

class Device;
class Pipeline;
class Window;

class Swapchain {
   public:
    Swapchain(std::shared_ptr<Device> device);
    ~Swapchain();

    bool Create(Window* window);
    void Destroy();

    bool CreateFramebuffers(Pipeline* pipeline);
    void DestroyFramebuffers();

    VkSurfaceFormatKHR surface_format() { return surface_format_; }
    VkExtent2D extent() { return extent_; }
    uint32_t image_count() const { return image_count_; }
    VkSwapchainKHR get() { return swapchain_; }
    VkFramebuffer framebuffer(size_t i) { return framebuffers_[i]; }

   private:
    std::shared_ptr<Device> device_;
    VkSurfaceFormatKHR surface_format_;
    VkPresentModeKHR present_mode_;
    VkExtent2D extent_;
    uint32_t image_count_;
    VkSwapchainKHR swapchain_;
    std::vector<VkImage> images_;
    std::vector<VkImageView> image_views_;
    std::vector<VkFramebuffer> framebuffers_;
};

}    // namespace vk

}    // namespace scin

#endif    // SRC_VULKAN_SWAP_CHAIN_H_

