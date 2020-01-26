include(ExternalProject)
set(SCIN_EXT_INSTALL_DIR "${PROJECT_BINARY_DIR}/install-ext" CACHE PATH "Scintallor Vulkan SDK ")
file(MAKE_DIRECTORY "${SCIN_EXT_INSTALL_DIR}")

include_directories("${SCIN_EXT_INSTALL_DIR}/include")

if (NOT SCIN_USE_OWN_VULKAN)
    # glslang is a dependency of the vulkan-validation layers so will have been built there if we were building our own.
    ExternalProject_Add(glslang
        PREFIX ext
        STEP_TARGETS install
        EXCLUDE_FROM_ALL ON
        GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
        GIT_TAG master
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        UPDATE_COMMAND ""
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${SCIN_EXT_INSTALL_DIR}
    )
    ExternalProject_Add_Step(glslang getdeps
        COMMAND ./update_glslang_sources.py
        WORKING_DIRECTORY "<SOURCE_DIR>"
        COMMENT "downloading glslang dependencies"
        DEPENDEES download
        DEPENDERS update
    )
endif()

#### shaderc
ExternalProject_Add(shaderc
    PREFIX ext
    STEP_TARGETS install
    EXCLUDE_FROM_ALL ON
    GIT_REPOSITORY https://github.com/google/shaderc
    GIT_TAG known-good
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    UPDATE_COMMAND ""
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${SCIN_EXT_INSTALL_DIR} -DSHADERC_SKIP_TESTS=ON -DSHADERC_ENABLE_NV_EXTENSIONS=OFF
    SOURCE_SUBDIR src/
)
ExternalProject_Add_Step(shaderc getdeps
    COMMAND ./update_shaderc_sources.py
    WORKING_DIRECTORY "<SOURCE_DIR>"
    COMMENT "downloading glslang dependencies"
    DEPENDEES download
    DEPENDERS update
)
if (NOT SCIN_USE_OWN_VULKAN)
    ExternalProject_Add_StepDependencies(shaderc build glslang-install)
endif()

#### fmt
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

#### yaml-cpp
add_subdirectory(yaml-cpp)
