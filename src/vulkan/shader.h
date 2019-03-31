#ifndef SRC_VULKAN_SHADER_H_
#define SRC_VULKAN_SHADER_H_

#include "vulkan/scin_include_vulkan.h"

#include <memory>
#include <string>

namespace scin {

namespace vk {

class Device;

class Shader {
  public:
    enum Kind { kVertex, kFragment };
    Shader(Kind kind, std::shared_ptr<Device> device, std::string entry_point);
    ~Shader();

    bool Create(const char* spv_bytes, size_t byte_size);
    void Destroy();

    VkShaderModule get() { return shader_module_; }
    const char* entry_point() const { return entry_point_.c_str(); }

  private:
    Kind kind_;
    std::shared_ptr<Device> device_;
    std::string entry_point_;
    VkShaderModule shader_module_;
};

}    // namespace vk

}    // namespace scin

#endif    // SRC_VULKAN_SHADER_H_

