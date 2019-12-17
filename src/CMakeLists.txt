set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

add_library(scintillator_core STATIC
    VGen.cpp
    VGen.hpp
    VGenManager.cpp
    VGenManager.hpp
)

target_link_libraries(scintillator_core
    spdlog
    yaml-cpp
    stdc++fs
)

configure_file(Version.h.in "${CMAKE_CURRENT_BINARY_DIR}/Version.h")
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/OscCommand.h"
    COMMAND gperf --output-file="${CMAKE_CURRENT_BINARY_DIR}/OscCommand.h" "${CMAKE_CURRENT_SOURCE_DIR}/OscCommand.h.in"
    MAIN_DEPENDENCY OscCommand.h.in
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

set(scintillator_synth_files
    "${CMAKE_CURRENT_BINARY_DIR}/Version.h"
    "${CMAKE_CURRENT_BINARY_DIR}/OscCommand.h"
    LogLevels.cpp
    LogLevels.h
    OscHandler.cpp
    OscHandler.hpp
    scinsynth.cc

    vulkan/Uniform.cpp
    vulkan/Uniform.hpp
    vulkan/buffer.cc
    vulkan/buffer.h
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
    SPIRV-Tools
    VulkanMemoryAllocator
    gflags
    glfw
    glm
    glslang
    oscpack
    scintillator_core
    shaderc
    spdlog
    stdc++fs
)

if(SCIN_USE_SWIFTSHADER)
    target_link_libraries(scinsynth
        SwiftShader
        vk_swiftshader
    )
else()
    find_package(Vulkan REQUIRED FATAL_ERROR)
    target_link_libraries(scinsynth "${Vulkan_LIBRARY}")
endif()


if(SCIN_VALIDATE_VULKAN)
    target_compile_definitions(scinsynth PRIVATE SCIN_VALIDATE_VULKAN)
endif()

add_executable(run_unittests
    VGenManager_unittests.cpp
    VGen_unittests.cpp

    run_unittests.cpp
)

target_link_libraries(run_unittests
    gtest
    scintillator_core
)
