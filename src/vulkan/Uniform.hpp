#ifndef SRC_VULKAN_UNIFORM_HPP_
#define SRC_VULKAN_UNIFORM_HPP_

#include "vulkan/Vulkan.hpp"

#include <memory>
#include <vector>

namespace scin {
namespace vk {

class Buffer;
class Device;
class Swapchain;

struct GlobalUniform {
    alignas(16) float time;
};

class Uniform {
public:
    Uniform(std::shared_ptr<Device> device, size_t size);
    ~Uniform();

    bool createLayout();
    bool createBuffers(Swapchain* swapchain);

    void destroy();

    VkDescriptorSetLayout* pLayout() { return &m_layout; }
    std::shared_ptr<Buffer> buffer(int index) { return m_buffers[index]; }
    VkDescriptorSet* set(int index) { return &m_sets[index]; }

private:
    std::shared_ptr<Device> m_device;
    size_t m_size;
    VkDescriptorSetLayout m_layout;

    std::vector<std::shared_ptr<Buffer>> m_buffers;
    VkDescriptorPool m_pool;
    std::vector<VkDescriptorSet> m_sets;
};

}  // namespace vk
}  // namespace scin

#endif  // SRC_VULKAN_UNIFORM_HPP_

