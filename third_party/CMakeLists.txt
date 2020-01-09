set(SCIN_EXT_INSTALL_DIR "${PROJECT_BINARY_DIR}/install-ext" CACHE PATH "where to install Scintillator dependencies")
file(MAKE_DIRECTORY ${SCIN_EXT_INSTALL_DIR})

# All the Khronos Group vulkan SDK releases are under the same git tag, so we keep it consistent with a variable.
set(SCIN_VULKAN_TAG "sdk-1.1.130")

include(ExternalProject)
ExternalProject_add(Vulkan-Headers
    PREFIX ext
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG ${SCIN_VULKAN_TAG}
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${SCIN_EXT_INSTALL_DIR}
)
ExternalProject_Add_StepTargets(Vulkan-Headers install)

#    include_directories(Vulkan-Headers/include)
#    add_subdirectory(Vulkan-Headers)

ExternalProject_add(Vulkan-Loader
    PREFIX ext
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Loader.git
    GIT_TAG ${SCIN_VULKAN_TAG}
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${SCIN_EXT_INSTALL_DIR} -DUSE_CCACHE=ON
)
ExternalProject_Add_StepDependencies(Vulkan-Loader install Vulkan-Headers)
ExternalProject_Add_StepTargets(Vulkan-Loader install)

#    add_subdirectory(Vulkan-Loader)

set(BUILD_SAMPLES OFF CACHE BOOL "don't build swiftshader samples" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "don't build swiftshader tests" FORCE)
set(BUILD_VULKAN ON CACHE BOOL "build Vulkan" FORCE)
set(BUILD_EGL OFF CACHE BOOL "don't build egl" FORCE)
set(BUILD_GLESV2 OFF CACHE BOOL "don't build GLESv2" FORCE)
set(BUILD_GLES_CM OFF CACHE BOOL "don't build GLES CM" FORCE)
# https://github.com/google/amber/issues/754
set(CMAKE_SHARED_LINKER_FLAGS "-fuse-ld=gold" CACHE STRING "workaround an issue in gnu ld that breaks vulkan build"
    FORCE)
add_subdirectory(swiftshader)

#### glslang (required by shaderc, Vulkan validation layers)
#add_subdirectory(glslang)
ExternalProject_Add(glslang
    PREFIX ext
    GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
    GIT_TAG master
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${SCIN_EXT_INSTALL_DIR}
)
ExternalProject_Add_Step(glslang GETDEPS
    COMMAND ./update_glslang_sources.py
    WORKING_DIRECTORY "<SOURCE_DIR>"
    COMMENT "downloading glslang dependencies"
    DEPENDEES download
    DEPENDERS configure
)

ExternalProject_Add(Vulkan-ValidationLayers
    PREFIX ext
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-ValidationLayers.git
    GIT_TAG ${SCIN_VULKAN_TAG}
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${SCIN_EXT_INSTALL_DIR} -DVULKAN_HEADERS_INSTALL_DIR=${SCIN_EXT_INSTALL_DIR} -DGLSLANG_INSTALL_DIR=${SCIN_EXT_INSTALL_DIR}
)
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
# TODO: still necessary?
include_directories("${SCIN_EXT_INSTALL_DIR}/include")

#### glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)
add_dependencies(glfw Vulkan-Headers-install)

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
#set(SHADERC_SKIP_TESTS ON CACHE BOOL "skip testing in shaderc")
#add_subdirectory(shaderc)
ExternalProject_Add(shaderc
    PREFIX ext
    GIT_REPOSITORY https://github.com/google/shaderc
    GIT_TAG known-good
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${SCIN_EXT_INSTALL_DIR} -DSHADERC_SKIP_TESTS=ON
    SOURCE_SUBDIR src/
)
ExternalProject_Add_Step(shaderc GETDEPS
    COMMAND ./update_shaderc_sources.py
    WORKING_DIRECTORY "<SOURCE_DIR>"
    COMMENT "downloading glslang dependencies"
    DEPENDEES download
    DEPENDERS configure
)
ExternalProject_Add_StepTargets(shaderc install)

#### yaml-cpp
add_subdirectory(yaml-cpp)

