#ifndef SRC_VULKAN_SHADER_SOURCE_HPP_
#define SRC_VULKAN_SHADER_SOURCE_HPP_

#include <string>

namespace scin { namespace vk {

// Wrapper object for source code for a shader, allowing for programmatic
// assembly of the shader source code.
// TODO: seems like a useless abstraction. Deprecate.
class ShaderSource {
public:
    ShaderSource(std::string name, std::string source);
    ~ShaderSource();

    const char* get() const { return source_.c_str(); }
    size_t size() const { return source_.size(); }
    const char* name() const { return name_.c_str(); }

    const char* entry_point() const { return "main"; }

private:
    std::string name_;
    std::string source_;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_SHADER_SOURCE_HPP_
