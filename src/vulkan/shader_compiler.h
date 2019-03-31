#ifndef SRC_VULKAN_SHADER_COMPILER_H_
#define SRC_VULKAN_SHADER_COMIPLER_H_

#include "shaderc/shaderc.h"

#include "vulkan/shader.h"

#include <memory>

namespace scin {

namespace vk {

class Device;
class ShaderSource;

// The shader compiler provided by libshaderc consumes some memory resources
// when loaded, so this object wraps the code needed by the shader compiler
// to control loading and unloading as well as compilation.
class ShaderCompiler {
  public:
    ShaderCompiler();
    ~ShaderCompiler();

    bool LoadCompiler();
    void ReleaseCompiler();

    std::unique_ptr<Shader> Compile(
            std::shared_ptr<Device> device,
            ShaderSource* source,
            Shader::Kind kind);

  private:
    shaderc_compiler_t compiler_;
};

}    // namespace vk

}    // namespace scin

#endif    // SRC_VULKAN_SHADER_COMPILER_H_

