#ifndef SRC_VULKAN_SWAP_CHAIN_H_
#define SRC_VULKAN_SWAP_CHAIN_H_

#include "scin_include_vulkan.h"

#include <memory>
#include <vector>

namespace scin {

namespace vk {

class Device;
class Window;

class Swapchain {
   public:
    Swapchain(std::shared_ptr<Device> device);
    ~Swapchain();

    bool Create(Window* window);
    void Destroy();

    VkExtent2D extent() { return extent_; }
    VkSurfaceFormatKHR surface_format() { return surface_format_; }

   private:
    std::shared_ptr<Device> device_;
    VkSurfaceFormatKHR surface_format_;
    VkPresentModeKHR present_mode_;
    VkExtent2D extent_;
    VkSwapchainKHR swapchain_;
    std::vector<VkImage> images_;
    std::vector<VkImageView> image_views_;
};

}    // namespace vk

}    // namespace scin

#endif    // SRC_VULKAN_SWAP_CHAIN_H_

