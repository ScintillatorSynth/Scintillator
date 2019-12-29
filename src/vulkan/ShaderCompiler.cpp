#include "vulkan/ShaderCompiler.hpp"

#include "vulkan/Device.hpp"
#include "vulkan/Shader.hpp"
#include "vulkan/ShaderSource.hpp"

#include "spdlog/spdlog.h"

namespace scin { namespace vk {

ShaderCompiler::ShaderCompiler(): m_compiler(nullptr) {}

ShaderCompiler::~ShaderCompiler() { releaseCompiler(); }

bool ShaderCompiler::loadCompiler() {
    if (!compilerLoaded()) {
        m_compiler = shaderc_compiler_initialize();
    }
    return (m_compiler != nullptr);
}

void ShaderCompiler::releaseCompiler() {
    if (compilerLoaded()) {
        shaderc_compiler_release(m_compiler);
        m_compiler = nullptr;
    }
}

std::unique_ptr<Shader> ShaderCompiler::compile(std::shared_ptr<Device> device, ShaderSource* source,
                                                Shader::Kind kind) {
    if (!compilerLoaded()) {
        if (!loadCompiler()) {
            return std::unique_ptr<Shader>();
        }
    }

    shaderc_shader_kind shaderKind;
    switch (kind) {
    case Shader::kVertex:
        shaderKind = shaderc_vertex_shader;
        break;

    case Shader::kFragment:
        shaderKind = shaderc_fragment_shader;
        break;
    }

    shaderc_compile_options_t options = shaderc_compile_options_initialize();
    if (!options) {
        return std::unique_ptr<Shader>();
    }

    shaderc_compilation_result_t result = shaderc_compile_into_spv(
        m_compiler, source->get(), source->size(), shaderKind, source->name(), source->entry_point(), options);

    std::unique_ptr<Shader> shader(nullptr);
    shaderc_compilation_status status = shaderc_result_get_compilation_status(result);
    if (status == shaderc_compilation_status_success) {
        const char* spv_bytes = shaderc_result_get_bytes(result);
        size_t byte_size = shaderc_result_get_length(result);
        shader.reset(new Shader(kind, device, source->entry_point()));
        if (!shader->Create(spv_bytes, byte_size)) {
            spdlog::error("error creating shader from compiled source {}.", source->name());
            shader.reset(nullptr);
        } else {
            spdlog::info("successfully compiled shader source {}.", source->name());
        }
    } else {
        spdlog::error("error compiling shader {}: {}", source->name(), shaderc_result_get_error_message(result));
    }

    shaderc_result_release(result);
    shaderc_compile_options_release(options);
    return shader;
}

} // namespace vk

} // namespace scin
