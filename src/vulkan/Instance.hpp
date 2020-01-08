#ifndef SRC_VULKAN_INSTANCE_HPP_
#define SRC_VULKAN_INSTANCE_HPP_

#include "vulkan/Vulkan.hpp"

namespace scin { namespace vk {

/*! The Vulkan Instance, VkInstance, is the primary access to the Vulkan API. This class encapsulates the instance,
 * manages creation and destruction. It also currently handles the validation layer setup and logging.
 */
class Instance {
public:
    Instance(bool enableValidation);
    ~Instance();

    /*! Attempts to create a Vulkan Instance, and set up Validation Layers, if configured.
     *
     * \return true on success, false on failure.
     */
    bool create();

    /*! Destroys the Vulkan Instance.
     */
    void destroy();

    VkInstance get() { return m_instance; }

private:
    bool m_enableValidation;
    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debugMessenger;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_INSTANCE_HPP_
