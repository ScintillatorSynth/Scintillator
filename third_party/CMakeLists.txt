set(SCIN_VULKAN_TAG "sdk-1.1.130")
include(ExternalProject)
ExternalProject_add(Vulkan-Headers
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG ${SCIN_VULKAN_TAG}
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}
)

#    include_directories(Vulkan-Headers/include)
#    add_subdirectory(Vulkan-Headers)

ExternalProject_add(Vulkan-Loader
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Loader.git
    GIT_TAG ${SCIN_VULKAN_TAG}
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}
)
ExternalProject_Add_StepDependencies(Vulkan-Loader install Vulkan-Headers)

#    add_subdirectory(Vulkan-Loader)

set(BUILD_SAMPLES OFF CACHE BOOL "don't build swiftshader samples" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "don't build swiftshader tests" FORCE)
set(BUILD_VULKAN ON CACHE BOOL "build Vulkan" FORCE)
set(BUILD_EGL OFF CACHE BOOL "don't build egl" FORCE)
set(BUILD_GLESV2 OFF CACHE BOOL "don't build GLESv2" FORCE)
set(BUILD_GLES_CM OFF CACHE BOOL "don't build GLES CM" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS "-fuse-ld=gold" CACHE STRING "workaround an issue in gnu ld that breaks vulkan build"
    FORCE)
add_subdirectory(swiftshader)

#### glslang (required by shaderc, Vulkan validation layers)
add_subdirectory(glslang)

#ExternalProject_add(Vulkan-ValidationLayers
#    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-ValidationLayers.git
#    GIT_TAG ${SCIN_VULKAN_TAG}
#    GIT_SHALLOW ON
#    GIT_PROGRESS ON
#    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}
#)
ExternalProject_Add_StepDependencies(Vulkan-ValidationLayers install Vulkan-Headers)
ExternalProject_Add_StepDependencies(Vulkan-ValidationLayers install Vulkan-Loader)
ExternalProject_Add_StepDependencies(Vulkan-ValidationLayers build glslang)
#    add_subdirectory(Vulkan-ValidationLayers)
# set(VulkanHeaders_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Vulkan-Headers/include" CACHE PATH "" FORCE)
# add_subdirectory(Vulkan-Tools)

### fmt
add_subdirectory(fmt)

#### gflags
set(GFLAGS_BUILD_STATIC_LIBS ON CACHE BOOL "build gflags as a static library")
set(GFLAGS_BUILD_gflags_LIB ON CACHE BOOL "build multithreaded gflags library")
add_subdirectory(gflags)

#### spdlog
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "require spdlog to use our external fmt")
add_subdirectory(spdlog)

# direct include of installed external deps header path
include_directories("${CMAKE_CURRENT_BINARY_DIR}/include")

#### glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)
add_dependencies(glfw Vulkan-Headers)

#### glm
add_subdirectory(glm)

#### VulkanMemoryAllocator
include_directories(VulkanMemoryAllocator/src)
add_library(VulkanMemoryAllocator STATIC "vulkan_memory_allocator_build.cc")
target_include_directories(VulkanMemoryAllocator INTERFACE VulkanMemoryAllocator/src)
add_dependencies(VulkanMemoryAllocator Vulkan-Headers)

#### googletest (required by effcee and re2)
add_subdirectory(googletest)

#### oscpack
include_directories(oscpack_1_1_0)
add_library(oscpack STATIC "OSCPackBuild.cpp")
target_include_directories(oscpack INTERFACE oscpack_1_1_0)

#TODO: remove submodule?
#### re2 (required by effcee and SPIRV-Tools)
#set(RE2_BUILD_TESTING OFF CACHE BOOL "build testing in re2")
#add_subdirectory(re2)

#TODO: remove submodule?
#### effcee (required by SPIRV-Tools)
#set(EFFCEE_BUILD_SAMPLES OFF CACHE BOOL "build samples in effcee")
#set(EFFCEE_BUILD_TESTING OFF CACHE BOOL "build testing in effcee")
#add_subdirectory(effcee)

#### shaderc
set(SHADERC_SKIP_TESTS ON CACHE BOOL "skip testing in shaderc")
add_subdirectory(shaderc)

#### yaml-cpp
add_subdirectory(yaml-cpp)

