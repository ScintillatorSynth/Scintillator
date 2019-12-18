#include "vulkan/ShaderSource.hpp"

namespace scin { namespace vk {

ShaderSource::ShaderSource(std::string name, std::string source): name_(name), source_(source) {}

ShaderSource::~ShaderSource() {}

} // namespace vk

} // namespace scin
