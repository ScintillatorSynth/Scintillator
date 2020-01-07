if(SCIN_USE_SYSTEM_VULKAN)
    find_package(Vulkan REQUIRED FATAL_ERROR)
else()
    add_subdirectory(Vulkan-Headers)
    set(BUILD_WSI_WAYLAND_SUPPORT OFF CACHE BOOL "don't build Wayland WSI support")
    add_subdirectory(Vulkan-Loader)
endif()

set(BUILD_SAMPLES OFF CACHE BOOL "don't build swiftshader samples" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "don't build swiftshader tests" FORCE)
set(BUILD_VULKAN ON CACHE BOOL "build Vulkan" FORCE)
set(BUILD_EGL OFF CACHE BOOL "don't build egl" FORCE)
set(BUILD_GLESV2 OFF CACHE BOOL "don't build GLESv2" FORCE)
set(BUILD_GLES_CM OFF CACHE BOOL "don't build GLES CM" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS "-fuse-ld=gold" CACHE STRING "workaround an issue in gnu ld that breaks vulkan build"
    FORCE)
add_subdirectory(swiftshader)

### fmt
add_subdirectory(fmt)

#### gflags
set(GFLAGS_BUILD_STATIC_LIBS ON CACHE BOOL "build gflags as a static library")
set(GFLAGS_BUILD_gflags_LIB ON CACHE BOOL "build multithreaded gflags library")
add_subdirectory(gflags)

#### spdlog
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "require spdlog to use our external fmt")
add_subdirectory(spdlog)

#### glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_VULKAN_STATIC ON CACHE BOOL "Link directly against the Vulkan loader" FORCE)
add_subdirectory(glfw)

#### glm
add_subdirectory(glm)

#### VulkanMemoryAllocator
include_directories(VulkanMemoryAllocator/src)
add_library(VulkanMemoryAllocator STATIC "vulkan_memory_allocator_build.cc")
target_include_directories(VulkanMemoryAllocator INTERFACE VulkanMemoryAllocator/src)

#### googletest (required by effcee and re2)
add_subdirectory(googletest)

#### oscpack
include_directories(oscpack_1_1_0)
add_library(oscpack STATIC "OSCPackBuild.cpp")
target_include_directories(oscpack INTERFACE oscpack_1_1_0)

#### re2 (required by effcee and SPIRV-Tools)
set(RE2_BUILD_TESTING OFF CACHE BOOL "build testing in re2")
add_subdirectory(re2)

#### effcee (required by SPIRV-Tools)
set(EFFCEE_BUILD_SAMPLES OFF CACHE BOOL "build samples in effcee")
set(EFFCEE_BUILD_TESTING OFF CACHE BOOL "build testing in effcee")
add_subdirectory(effcee)

#### SPIRV-Headers (required by SPIRV-Tools)
add_subdirectory(SPIRV-Headers)

#### SPIRV-Tools (required by shaderc)
if(NOT SCIN_USE_SWIFTSHADER) # swiftshader includes its own version of SPRIV-Tools
    set(SPIRV_SKIP_TESTS ON CACHE BOOL "skip testing in SPIRV-Tools")
    add_subdirectory(SPIRV-Tools)
endif()

#### glslang (required by shaderc)
add_subdirectory(glslang)

#### shaderc
set(SHADERC_SKIP_TESTS ON CACHE BOOL "skip testing in shaderc")
add_subdirectory(shaderc)

#### yaml-cpp
add_subdirectory(yaml-cpp)

