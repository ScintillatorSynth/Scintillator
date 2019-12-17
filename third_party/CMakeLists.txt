if(SCIN_USE_SWIFTSHADER)
    # Swiftshader can provide the Vulkan and GL headers to the other libraries.
    include_directories(swiftshader/include)
    set(BUILD_SAMPLES OFF CACHE BOOL "don't build swiftshader samples")
    set(BUILD_TESTS OFF CACHE BOOL "don't build swiftshader tests")
    add_subdirectory(swiftshader)
endif()

#### gflags
set(GFLAGS_BUILD_STATIC_LIBS ON CACHE BOOL "build gflags as a static library")
set(GFLAGS_BUILD_gflags_LIB ON CACHE BOOL "build multithreaded gflags library")
add_subdirectory(gflags)

#### spdlog
add_subdirectory(spdlog)

#### glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
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

