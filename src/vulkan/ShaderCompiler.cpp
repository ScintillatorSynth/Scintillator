#include "vulkan/ShaderCompiler.hpp"

#include "vulkan/Device.hpp"

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

std::unique_ptr<Shader> ShaderCompiler::compile(std::shared_ptr<Device> device, const std::string& source,
                                                const std::string& name, const std::string& entryPoint,
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

    // Swiftshader currently assumes shaders are compiled in Vulkan 1.1 environment and may crash when constructing
    // Pipelines that aren't built in that environment (https://issuetracker.google.com/issues/127454276).
    shaderc_compile_options_set_target_env(options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);

    shaderc_compilation_result_t result = shaderc_compile_into_spv(m_compiler, source.data(), source.size(), shaderKind,
                                                                   name.data(), entryPoint.data(), options);

    std::unique_ptr<Shader> shader(nullptr);
    shaderc_compilation_status status = shaderc_result_get_compilation_status(result);
    if (status == shaderc_compilation_status_success) {
        const char* spv_bytes = shaderc_result_get_bytes(result);
        size_t byte_size = shaderc_result_get_length(result);
        shader.reset(new Shader(kind, device, entryPoint));
        if (!shader->create(spv_bytes, byte_size)) {
            spdlog::error("error creating shader from compiled source {}.", name);
            shader.reset(nullptr);
        } else {
            if (shaderc_result_get_num_warnings(result)) {
                spdlog::warn("shaderc compiled but returned some warnings for source {}: {}.", name,
                             shaderc_result_get_error_message(result));
            } else {
                spdlog::info("successfully compiled shader source {}.", name);
            }
        }
    } else {
        spdlog::error("error compiling shader {}: {}", name, shaderc_result_get_error_message(result));
    }

    shaderc_result_release(result);
    shaderc_compile_options_release(options);
    return shader;
}

} // namespace vk

} // namespace scin
