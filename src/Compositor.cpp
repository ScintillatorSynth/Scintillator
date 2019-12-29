#include "Compositor.hpp"

#include "vulkan/CommandPool.hpp"
#include "vulkan/ShaderCompiler.hpp"

#include "spdlog/spdlog.h"

namespace scin {

Compositor::Compositor(std::shared_ptr<vk::Device> device) :
    m_device(device),
    m_shaderCompiler(new scin::vk::ShaderCompiler()),
    m_commandPool(new scin::vk::CommandPool(device)) { }

Compositor::~Compositor() { }

bool Compositor::create() {
    if (!m_shaderCompiler->loadCompiler()) {
        spdlog::error("unable to load shader compiler.");
        return false;
    }

    if (!m_commandPool->create()) {
        spdlog::error("error creating command pool.");
        return false;
    }

    // The compositor can be the keeper of device-specific shared resources. Such as - the shared vertex buffers, and
    // the shared index buffer. So we create those now.

    return true;
}

bool Compositor::buildScinthDef(std::shared_ptr<const AbstractScinthDef> abstractScinthDef) {

}

void Compositor::releaseCompiler() {
    m_shaderCompiler->releaseCompiler();
}

} // namespace scin
