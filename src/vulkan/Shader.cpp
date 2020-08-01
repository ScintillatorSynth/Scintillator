#include "vulkan/Shader.hpp"

#include "vulkan/Device.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace vk {

Shader::Shader(std::shared_ptr<Device> device, const std::string& entryPoint):
    m_device(device),
    m_entryPoint(entryPoint),
    m_shaderModule(VK_NULL_HANDLE) {}

Shader::~Shader() { destroy(); }

bool Shader::create(const char* spvBytes, size_t byteSize) {
    if (spvBytes == nullptr || byteSize == 0) {
        spdlog::error("Failed to create empty shader.");
        return false;
    }
    // Copy the shader bytes to our own memory. Shaderc guarantees the pointer is castable to a uint32_t* if
    // bytecode compliation was requested.
    m_spvBytes.reset(new uint32_t[byteSize / sizeof(uint32_t)]);
    std::memcpy(m_spvBytes.get(), spvBytes, byteSize);
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = byteSize;
    createInfo.pCode = m_spvBytes.get();
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
