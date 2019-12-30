#include "Compositor.hpp"

#include "ScinthDef.hpp"
#include "core/AbstractScinthDef.hpp"
#include "core/VGen.hpp"
#include "vulkan/CommandPool.hpp"
#include "vulkan/ShaderCompiler.hpp"

#include "spdlog/spdlog.h"

namespace scin {

Compositor::Compositor(std::shared_ptr<vk::Device> device):
    m_device(device),
    m_shaderCompiler(new scin::vk::ShaderCompiler()),
    m_commandPool(new scin::vk::CommandPool(device)) {}

Compositor::~Compositor() {}

bool Compositor::create() {
    if (!m_shaderCompiler->loadCompiler()) {
        spdlog::error("unable to load shader compiler.");
        return false;
    }

    if (!m_commandPool->create()) {
        spdlog::error("error creating command pool.");
        return false;
    }

    return true;
}

bool Compositor::buildScinthDef(std::shared_ptr<const AbstractScinthDef> abstractScinthDef) {
    std::unique_ptr<ScinthDef> scinthDef(new ScinthDef(m_device, abstractScinthDef));
    if (!scinthDef->build(m_shaderCompiler)) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_scinthDefs.insert_or_assign(abstractScinthDef->name(), std::move(scinthDef));
    }

    return true;
}

void Compositor::releaseCompiler() { m_shaderCompiler->releaseCompiler(); }

void Compositor::destroy() {}

} // namespace scin
