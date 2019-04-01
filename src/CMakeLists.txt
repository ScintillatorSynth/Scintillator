
#### Vulkan
find_package(Vulkan)
if (NOT VULKAN_FOUND)
    message(FATAL_ERROR "Vulkan not found")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(scintillator_synth_files
    scinsynth.cc
    vulkan/command_pool.cc
    vulkan/command_pool.h
    vulkan/device.cc
    vulkan/device.h
    vulkan/instance.cc
    vulkan/instance.h
    vulkan/pipeline.cc
    vulkan/pipeline.h
    vulkan/scin_include_vulkan.h
    vulkan/shader.cc
    vulkan/shader.h
    vulkan/shader_compiler.cc
    vulkan/shader_compiler.h
    vulkan/shader_source.cc
    vulkan/shader_source.h
    vulkan/swapchain.cc
    vulkan/swapchain.h
    vulkan/window.cc
    vulkan/window.h
)

add_executable(scinsynth ${scintillator_synth_files})

target_link_libraries(scinsynth
    ${VULKAN_LIBRARY}
    SPIRV-Tools
    glfw
    glslang
    shaderc
)

if (SCIN_VALIDATE_VULKAN)
    target_compile_definitions(scinsynth PRIVATE SCIN_VALIDATE_VULKAN)
endif()

