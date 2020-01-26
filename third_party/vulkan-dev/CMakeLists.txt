# Standalone cmake project to build and install Vulkan pre-requisites (for systems that may not provide them like
# Travis CI) and development tools.

# TODO: everything here but swiftshader is a function of SCIN_USE_OWN_VULKAN

include(ExternalProject)
cmake_policy(SET CMP0048 NEW)
cmake_minimum_required(VERSION 3.9)

project(vulkan-dev VERSION 0.0.1)

# if the Scintillator main build directory was not manually specified, attempt to guess it from repository structure.
if (NOT SCIN_BUILD_DIR)
    get_filename_component(SCIN_BUILD_DIR "../../build" ABSOLUTE ${PROJECT_SOURCE_DIR})
endif()

message(STATUS "Scintillator build directory set to: ${SCIN_BUILD_DIR}")

set(INSTALL_EXT_DIR "${PROJECT_BINARY_DIR}/install-ext" CACHE PATH "")

install(DIRECTORY ${INSTALL_EXT_DIR} DESTINATION ${SCIN_BUILD_DIR})

file(MAKE_DIRECTORY "${INSTALL_EXT_DIR}/share/vulkan/icd.d")
file(MAKE_DIRECTORY "${INSTALL_EXT_DIR}/share/vulkan/explicit_layer.d")

# Note: cmake-generated unix Makefiles break when using parallel builds, the combination of custom targets and
# ExternalProject cause multiple threads evaluating the external projects at the same time, causing flaky builds and
# breakage. To work around this these projects are specified in a single-child dependency chain, like:
# Vulkan-Headers <- Vulkan-Loader <- glslang <- Vulkan-ValidationLayers.
# Also, each dependent project is excluded from the "all" build target, because that was another implicit dependency
# that causes duplicate build jobs to execute There's a bug about this on cmake:
# https://gitlab.kitware.com/cmake/cmake/issues/18663
# TODO: consider using ninja, which doesn't have this problem, and may speed up the build.

if (SCIN_USE_OWN_VULKAN)
# All the Khronos Group vulkan SDK releases are under the same git tag, so we keep it consistent with a variable.
set(SCIN_VULKAN_TAG "sdk-1.1.130")

#### Vulkan Headers
ExternalProject_add(Vulkan-Headers
    PREFIX ext
    STEP_TARGETS install
    EXCLUDE_FROM_ALL ON
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG ${SCIN_VULKAN_TAG}
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    UPDATE_COMMAND ""
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${INSTALL_EXT_DIR}
)

#### Vulkan Loader
ExternalProject_add(Vulkan-Loader
    PREFIX ext
    DEPENDS Vulkan-Headers-install
    STEP_TARGETS install
    EXCLUDE_FROM_ALL ON
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Loader.git
    GIT_TAG ${SCIN_VULKAN_TAG}
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    UPDATE_COMMAND ""
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${INSTALL_EXT_DIR} -DUSE_CCACHE=ON
)

#### glslang (dependency of Vulkan Validation Layers)
ExternalProject_Add(glslang
    PREFIX ext
    DEPENDS Vulkan-Loader-install
    STEP_TARGETS install
    EXCLUDE_FROM_ALL ON
    GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
    GIT_TAG master
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    UPDATE_COMMAND ""
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${INSTALL_EXT_DIR}
)
ExternalProject_Add_Step(glslang getdeps
    COMMAND ./update_glslang_sources.py
    WORKING_DIRECTORY "<SOURCE_DIR>"
    COMMENT "downloading glslang dependencies"
    DEPENDEES download
    DEPENDERS update
)

#### Vulkan Validation Layers
# Note: keeping this in the "all" target means that "make install" will build the local Vulkan SDK as a dependency, if
# SCIN_USE_OWN_VULKAN is defined, while avoiding the parallel build issues detailed the top of this file.
ExternalProject_Add(Vulkan-ValidationLayers
    PREFIX ext
    DEPENDS glslang-install
    STEP_TARGETS install
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-ValidationLayers.git
    GIT_TAG ${SCIN_VULKAN_TAG}
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    UPDATE_COMMAND ""
    CMAKE_ARGS -DVULKAN_HEADERS_INSTALL_DIR=${INSTALL_EXT_DIR} -DGLSLANG_INSTALL_DIR=${INSTALL_EXT_DIR} -DUSE_CCACHE=ON
    INSTALL_COMMAND cp "<BINARY_DIR>/layers/libVkLayer_khronos_validation.so" "<BINARY_DIR>/layers/VkLayer_khronos_validation.json" "<BINARY_DIR>/layers/VkLayer_standard_validation.json" "${INSTALL_EXT_DIR}/share/vulkan/explicit_layer.d"
)
endif() # SCIN_USE_OWN_VULKAN

#### Swiftshader (Linux-only Vulkan software renderer)
# See https://github.com/google/amber/issues/754 for discussion of shared linker flag.
ExternalProject_Add(swiftshader
    PREFIX ext
    STEP_TARGETS install
    EXCLUDE_FROM_ALL
    GIT_REPOSITORY https://github.com/google/swiftshader
    GIT_TAG cd55f0518d6807f8505fcff11bd18044fcc191ee
    GIT_PROGRESS ON
    UPDATE_COMMAND ""
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Debug -DBUILD_SAMPLES=OFF -DBUILD_TESTS=OFF -DBUILD_VULKAN=ON -DBUILD_EGL=OFF -DBUILD_GLESV2=OFF -DBUILD_GLES_CM=OFF -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=gold"
    INSTALL_COMMAND cp "<BINARY_DIR>/Linux/libvk_swiftshader.so" "<BINARY_DIR>/Linux/vk_swiftshader_icd.json" "${INSTALL_EXT_DIR}/share/vulkan/icd.d"
)

# For Vulkan to pick up the Validation Layer and swiftshader they need to be installed in ${HOME}/.local/share/vulkan
# so this target will do that.
add_custom_target(local-vulkan-utils)
add_dependencies(local-vulkan-utils
    swiftshader-install
)
if (SCIN_USE_OWN_VULKAN)
    add_dependencies(local-vulkan-utils
        Vulkan-ValidationLayers-install
    )
    add_custom_command(TARGET local-vulkan-utils
        POST_BUILD
        COMMAND mkdir -p "$ENV{HOME}/.local/share/vulkan"
        COMMAND cp -R "${INSTALL_EXT_DIR}/share/vulkan/explicit_layer.d" "$ENV{HOME}/.local/share/vulkan/."
        COMMENT "installing Vulkan Validation Layers in $ENV{HOME}/.local/share/vulkan/explicit_layer.d"
        VERBATIM
    )
endif()

add_custom_command(TARGET local-vulkan-utils
    POST_BUILD
    COMMAND mkdir -p "$ENV{HOME}/.local/share/vulkan"
    COMMAND cp -R "${INSTALL_EXT_DIR}/share/vulkan/icd.d" "$ENV{HOME}/.local/share/vulkan/."
    COMMENT "installing swiftshader in $ENV{HOME}/.local/share/vulkan/icd.d"
    VERBATIM
)
