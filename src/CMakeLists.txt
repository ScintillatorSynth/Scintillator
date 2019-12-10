#### Vulkan
find_package(Vulkan REQUIRED FATAL_ERROR)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

configure_file(Version.h.in "${CMAKE_CURRENT_BINARY_DIR}/Version.h")
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/OscCommand.h"
    COMMAND gperf --output-file="${CMAKE_CURRENT_BINARY_DIR}/OscCommand.h" "${CMAKE_CURRENT_SOURCE_DIR}/OscCommand.h.in"
    MAIN_DEPENDENCY OscCommand.h.in
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

set(scintillator_synth_files
    "${CMAKE_CURRENT_BINARY_DIR}/Version.h"
    "${CMAKE_CURRENT_BINARY_DIR}/OscCommand.h"
    OscHandler.cpp
    OscHandler.hpp
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
    "${Vulkan_LIBRARY}"
    SPIRV-Tools
    gflags
    glfw
    glslang
    oscpack
    shaderc
)

if (SCIN_VALIDATE_VULKAN)
    target_compile_definitions(scinsynth PRIVATE SCIN_VALIDATE_VULKAN)
endif()

