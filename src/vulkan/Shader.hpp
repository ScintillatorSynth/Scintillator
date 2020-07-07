#ifndef SRC_VULKAN_SHADER_HPP_
#define SRC_VULKAN_SHADER_HPP_

#include "vulkan/Vulkan.hpp"

#include <memory>
#include <string>

namespace scin { namespace vk {

class Device;

class Shader {
public:
    enum Kind { kCompute, kVertex, kFragment };
    Shader(Kind kind, std::shared_ptr<Device> device, const std::string& entryPoint);
    ~Shader();

    bool create(const char* spvBytes, size_t byteSize);
    void destroy();

    VkShaderModule get() { return m_shaderModule; }
    const char* entryPoint() const { return m_entryPoint.data(); }

private:
    Kind m_kind;
    std::shared_ptr<Device> m_device;
    std::string m_entryPoint;
    std::unique_ptr<uint32_t[]> m_spvBytes;
    VkShaderModule m_shaderModule;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_SHADER_HPP_
