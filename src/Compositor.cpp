#include "Compositor.hpp"

#include "vulkan/ShaderCompiler.hpp"

#include "spdlog/spdlog.h"

namespace scin {

Compositor::Compositor(std::shared_ptr<vk::Device> device) :
    m_device(device),
    m_shaderCompiler(new scin::vk::ShaderCompiler()) { }

Compositor::~Compositor() { }

bool Compositor::create() {
    if (!m_shaderCompiler->loadCompiler()) {
        spdlog::error("unable to load shader compiler.");
        return false;
    }

    return true;

}

void Compositor::releaseCompiler() {
    m_shaderCompiler->releaseCompiler();
}

} // namespace scin
