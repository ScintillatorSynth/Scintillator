#include "vulkan/Shader.hpp"

#include "vulkan/Device.hpp"

namespace scin { namespace vk {

Shader::Shader(Kind kind, std::shared_ptr<Device> device, const std::string& entryPoint):
    m_kind(kind),
    m_device(device),
    m_entryPoint(entryPoint),
    m_shaderModule(VK_NULL_HANDLE) {}

Shader::~Shader() { destroy(); }

bool Shader::create(const char* spvBytes, size_t byteSize) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = byteSize;
    createInfo.pCode = reinterpret_cast<const uint32_t*>(spvBytes);
    return (vkCreateShaderModule(m_device->get(), &createInfo, nullptr, &m_shaderModule) == VK_SUCCESS);
}

void Shader::destroy() {
    if (m_shaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(m_device->get(), m_shaderModule, nullptr);
        m_shaderModule = VK_NULL_HANDLE;
    }
}

} // namespace vk

} // namespace scin
