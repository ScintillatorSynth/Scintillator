#include "vulkan/shader_source.h"

namespace scin {

namespace vk {

ShaderSource::ShaderSource(std::string source, std::string name) :
    source_(source),
    name_(name) {
}

ShaderSource::~ShaderSource() {
}

}    // namespace vk

}    // namespace scin
