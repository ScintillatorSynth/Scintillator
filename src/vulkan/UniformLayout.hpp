#ifndef SRC_VULKAN_UNIFORM_LAYOUT_HPP_
#define SRC_VULKAN_UNIFORM_LAYOUT_HPP_

#include "vulkan/Vulkan.hpp"

#include <memory>

namespace scin { namespace vk {

class Device;

/*! Wraps a description of uniform variables layout in Vulkan. Right now assumes there is a single uniform buffer,
 * provided only to the fragment shader.
 */
class UniformLayout {
public:
    UniformLayout(std::shared_ptr<Device> device);
    ~UniformLayout();

    bool create();
    void destroy();

    VkDescriptorSetLayout get() const { return m_layout; }
    const VkDescriptorSetLayout* getPointer() const { return &m_layout; }

private:
    std::shared_ptr<Device> m_device;
    VkDescriptorSetLayout m_layout;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_UNIFORM_LAYOUT_HPP_
