#ifndef SRC_VULKAN_SWAP_CHAIN_H_
#define SRC_VULKAN_SWAP_CHAIN_H_

#include "scin_include_vulkan.h"

namespace scin {

class VulkanDevice;

class VulkanSwapChain {
   public:
    VulkanSwapChain();
    ~VulkanSwapChain();

    bool Create(VulkanDevice* device);
    void Destroy();

   private:
    VkSurfaceFormatKHR surface_format_;
    VkPresentModeKHR present_mode_;
    VkExtent2D extent_;
};

}  // namespace scin

#endif  // SRC_VULKAN_SWAP_CHAIN_H_

