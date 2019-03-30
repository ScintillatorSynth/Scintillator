#ifndef SRC_VULKAN_SWAP_CHAIN_H_
#define SRC_VULKAN_SWAP_CHAIN_H_

#include "scin_include_vulkan.h"

#include <memory>
#include <vector>

namespace scin {

namespace vk {

class Device;
class Window;

class SwapChain {
   public:
    SwapChain(std::shared_ptr<Device> device);
    ~SwapChain();

    bool Create(Window* window);
    void Destroy();

   private:
    std::shared_ptr<Device> device_;
    VkSurfaceFormatKHR surface_format_;
    VkPresentModeKHR present_mode_;
    VkExtent2D extent_;
    // TODO: rename stuff to reflect fact that Vulkan thinks "swapchain" is
    // a single word.
    VkSwapchainKHR swap_chain_;
    std::vector<VkImage> images_;
    std::vector<VkImageView> image_views_;
};

}    // namespace vk

}    // namespace scin

#endif    // SRC_VULKAN_SWAP_CHAIN_H_

