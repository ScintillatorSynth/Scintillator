include(ExternalProject)
set(SCIN_EXT_INSTALL_DIR "${PROJECT_BINARY_DIR}/install-ext" CACHE PATH "where to install Scintillator dependencies")
file(MAKE_DIRECTORY ${SCIN_EXT_INSTALL_DIR})

# All the Khronos Group vulkan SDK releases are under the same git tag, so we keep it consistent with a variable.
set(SCIN_VULKAN_TAG "sdk-1.1.130")

#### Vulkan Headers
ExternalProject_add(Vulkan-Headers
    PREFIX ext
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG ${SCIN_VULKAN_TAG}
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${SCIN_EXT_INSTALL_DIR}
)
ExternalProject_Add_StepTargets(Vulkan-Headers install)

#### Vulkan Loader
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

# See https://github.com/google/amber/issues/754 for discussion of shared linker flag.
ExternalProject_Add(swiftshader
    PREFIX ext
    GIT_REPOSITORY https://github.com/google/swiftshader
    GIT_TAG master
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${SCIN_EXT_INSTALL_DIR} -DBUILD_SAMPLES=OFF -DBUILD_TESTS=OFF -DBUILD_VULKAN=ON -DBUILD_EGL=OFF -DBUILD_GLESV2=OFF -DBUILD_GLES_CM=OFF -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=gold"
)

#### glslang (required by shaderc, Vulkan validation layers)
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

#### Vulkan Validation Layers
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

#### fmt
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

#### shaderc
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

