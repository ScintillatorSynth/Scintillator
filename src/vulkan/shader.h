#ifndef SRC_VULKAN_SHADER_H_
#define SRC_VULKAN_SHADER_H_

#include "vulkan/scin_include_vulkan.h"

#include <memory>

namespace scin {

namespace vk {

class Device;

class Shader {
  public:
    enum Kind { kVertex, kFragment };
    Shader(Kind kind, std::shared_ptr<Device> device);
    ~Shader();

    bool Create(const char* spv_bytes, size_t byte_size);
    void Destroy();

  private:
    Kind kind_;
    std::shared_ptr<Device> device_;
    VkShaderModule shader_module_;
};

}    // namespace vk

}    // namespace scin

#endif    // SRC_VULKAN_SHADER_H_

