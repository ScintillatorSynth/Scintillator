#include "vulkan/shader_compiler.h"

#include "vulkan/shader.h"
#include "vulkan/shader_source.h"

#include <cstring>
#include <iostream>

namespace scin {

namespace vk {

ShaderCompiler::ShaderCompiler() :
    compiler_(nullptr) {
}

ShaderCompiler::~ShaderCompiler() {
}

bool ShaderCompiler::LoadCompiler() {
    if (compiler_ == nullptr) {
        compiler_ = shaderc_compiler_initialize();
    }
    return (compiler_ != nullptr);
}

void ShaderCompiler::ReleaseCompiler() {
    if (compiler_ != nullptr) {
        shaderc_compiler_release(compiler_);
        compiler_ = nullptr;
    }
}

std::unique_ptr<Shader> ShaderCompiler::Compile(
    ShaderSource* source, Shader::Kind kind) {
    if (compiler_ == nullptr) {
        if (!LoadCompiler()) {
            return std::unique_ptr<Shader>();
        }
    }

    shaderc_shader_kind shader_kind;
    switch (kind) {
        case Shader::kVertex:
            shader_kind = shaderc_vertex_shader;
            break;

        case Shader::kFragment:
            shader_kind = shaderc_fragment_shader;
            break;
    }

    shaderc_compile_options_t options = shaderc_compile_options_initialize();
    if (!options) {
        return std::unique_ptr<Shader>();
    }

    shaderc_compilation_result_t result = shaderc_compile_into_spv(
            compiler_,
            source->get(),
            source->size(),
            shader_kind,
            source->name(),
            source->entry_point(),
            options);

    std::unique_ptr<Shader> shader(nullptr);

    if (shaderc_result_get_compilation_status(result)) {
        size_t byte_size = shaderc_result_get_length(result);
        std::unique_ptr<char[]> spv_bytes(new char[byte_size]);
        const char* bytes = shaderc_result_get_bytes(result);
        std::memcpy(spv_bytes.get(), bytes, byte_size);
        shader.reset(new Shader(kind, std::move(spv_bytes), byte_size));
    } else {
        std::cerr << shaderc_result_get_error_message(result) << std::endl;
    }

    shaderc_result_release(result);
    shaderc_compile_options_release(options);
    return shader;
}

}    // namespace vk

}    // namespace scin

