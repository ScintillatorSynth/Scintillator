#ifndef SRC_VULKAN_SHADER_COMPILER_HPP_
#define SRC_VULKAN_SHADER_COMPILER_HPP_

#include "shaderc/shaderc.h"

#include "vulkan/Shader.hpp"

#include <memory>
#include <string>

namespace scin { namespace vk {

class Device;
class Shader;

/*! Provides a wrapper around the libshaderc compiler.
 *
 * The shader compiler provided by libshaderc consumes some memory resources when loaded, so this object wraps the code
 * needed by the shader compiler to control loading and unloading as well as compilation.
 */
class ShaderCompiler {
public:
    ShaderCompiler();
    ~ShaderCompiler();

    /*! Loads the shader compiler in to memory.
     */
    bool loadCompiler();

    /*! Releases the shader compiler from memory.
     *
     * \note Subsequent calls to compile() will re-load the compiler, adding time to compilation as it loads.
     */
    void releaseCompiler();

    /*! Returns true if compiler is loaded, false if not.
     *
     * \return True if compiler currently loaded. False if not.
     */
    bool compilerLoaded() const { return m_compiler != nullptr; }

    /*! Compile the provided shader source code into a usable Shader on the provided Device.
     *
     * \param device A shared pointer to the specific Vulkan Device to compile this Shader for.
     * \param source The source code of the shader to compile.
     * \param name The name of the source program to compile.
     * \param entryPoint The name of the function to call as the entry point to the shader program (typically "main").
     * \param kind The kind of shader to compile this as, e.g. Shader::kVertex or Shader::kFragment.
     * \return A pointer to the compiled Shader, or nullptr on error.
     */
    std::unique_ptr<Shader> compile(std::shared_ptr<Device> device, const std::string& source, const std::string& name,
                                    const std::string& entryPoint, Shader::Kind kind);

private:
    shaderc_compiler_t m_compiler;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_SHADER_COMPILER_HPP_
