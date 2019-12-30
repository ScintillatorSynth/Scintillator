#include "ScinthDef.hpp"

#include "Scinth.hpp"
#include "core/AbstractScinthDef.hpp"
#include "core/VGen.hpp"
#include "vulkan/CommandPool.hpp"
#include "vulkan/ShaderCompiler.hpp"

#include "spdlog/spdlog.h"

namespace scin {

ScinthDef::ScinthDef(std::shared_ptr<vk::Device> device, std::shared_ptr<const AbstractScinthDef> abstractScinthDef):
    m_device(device),
    m_abstractScinthDef(abstractScinthDef) {}

ScinthDef::~ScinthDef() {}

bool ScinthDef::build(std::shared_ptr<vk::ShaderCompiler> compiler) {
    m_vertexShader = compiler->compile(m_device, m_abstractScinthDef->vertexShader(),
            m_abstractScinthDef->uniquePrefix() + "_vertexShader", "main", vk::Shader::kVertex);
    if (!m_vertexShader) {
        spdlog::error("error compiling vertex shader for ScinthDef {}.", m_abstractScinthDef->name());
        return false;
    }

    m_fragmentShader = compiler->compile(m_device, m_abstractScinthDef->fragmentShader(),
            m_abstractScinthDef->uniquePrefix() + "_fragmentShader", "main", vk::Shader::kFragment);
    if (!m_fragmentShader) {
        spdlog::error("error compiling fragment shader for ScinthDef {}", m_abstractScinthDef->name());
        return false;
    }

    m_pipeline.reset(new vk::Pipeline(m_device));

    return true;
}

std::unique_ptr<Scinth> ScinthDef::play(std::shared_ptr<vk::CommandPool> comandPool) { return nullptr; }

} // namespace scin
