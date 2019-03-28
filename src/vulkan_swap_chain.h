#ifndef SRC_VULKAN_SWAP_CHAIN_H_
#define SRC_VULKAN_SWAP_CHAIN_H_

#include "scin_include_vulkan.h"

#include <memory>
#include <vector>

namespace scin {

class VulkanDevice;
class VulkanWindow;

class VulkanSwapChain {
   public:
    VulkanSwapChain(std::shared_ptr<VulkanDevice> device);
    ~VulkanSwapChain();

    bool Create(VulkanWindow* window);
    void Destroy();

   private:
    std::shared_ptr<VulkanDevice> device_;
    VkSurfaceFormatKHR surface_format_;
    VkPresentModeKHR present_mode_;
    VkExtent2D extent_;
    // TODO: rename stuff to reflect fact that Vulkan thinks "swapchain" is
    // a single word.
    VkSwapchainKHR swap_chain_;
    std::vector<VkImage> images_;
    std::vector<VkImageView> image_views_;
};

}  // namespace scin

#endif  // SRC_VULKAN_SWAP_CHAIN_H_

