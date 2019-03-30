#include "vulkan/shader.h"

#include "vulkan/device.h"

namespace scin {

namespace vk {

Shader::Shader(Kind kind, std::unique_ptr<char[]> spv_bytes, size_t byte_size) :
    kind_(kind),
    spv_bytes_(std::move(spv_bytes)),
    byte_size_(byte_size),
    shader_module_(VK_NULL_HANDLE) {
}

Shader::~Shader() {
}

}    // namespace vk

}    // namespace scin
