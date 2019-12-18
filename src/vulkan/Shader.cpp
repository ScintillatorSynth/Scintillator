#include "vulkan/Shader.hpp"

#include "vulkan/Device.hpp"

namespace scin {

namespace vk {

Shader::Shader(Kind kind, std::shared_ptr<Device> device,
        std::string entry_point) :
    kind_(kind),
    device_(device),
    entry_point_(entry_point),
    shader_module_(VK_NULL_HANDLE) {
}

bool Shader::Create(const char* spv_bytes, size_t byte_size) {
    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = byte_size;
    create_info.pCode = reinterpret_cast<const uint32_t*>(spv_bytes);
    return (vkCreateShaderModule(device_->get(), &create_info, nullptr,
            &shader_module_) == VK_SUCCESS);
}

void Shader::Destroy() {
    vkDestroyShaderModule(device_->get(), shader_module_, nullptr);
    shader_module_ = VK_NULL_HANDLE;
}

Shader::~Shader() {
    if (shader_module_ != VK_NULL_HANDLE) {
        Destroy();
    }
}

}    // namespace vk

}    // namespace scin

