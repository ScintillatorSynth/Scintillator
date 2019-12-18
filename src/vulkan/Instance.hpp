#ifndef SRC_VULKAN_INSTANCE_H_
#define SRC_VULKAN_INSTANCE_H_

#include "vulkan/Vulkan.hpp"

namespace scin {

namespace vk {

// The Vulkan Instance, VkInstance, is the primary access to the Vulkan API.
// This class encapsulates the instance, manages creation and destruction. It
// also currently handles the validation layer setup and logging.
class Instance {
 public:
    Instance();
    ~Instance();

    // Attempt to create the Vulkan Instance, will return true on success.
    bool Create();
    void Destroy();

    VkInstance get() { return instance_; }

 private:
    VkInstance instance_;

#if defined(SCIN_VALIDATE_VULKAN)
    VkDebugUtilsMessengerEXT debug_messenger_;
#endif
};

}    // namespace vk

}    // namespace scin

#endif    // SRC_VULKAN_INSTANCE_H_
