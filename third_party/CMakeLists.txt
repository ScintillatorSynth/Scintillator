set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CXX_EXTENSIONS OFF)

include(ExternalProject)
set(SCIN_EXT_INSTALL_DIR "${PROJECT_BINARY_DIR}/install-ext" CACHE PATH "Scintallor Vulkan SDK ")
file(MAKE_DIRECTORY "${SCIN_EXT_INSTALL_DIR}")
file(MAKE_DIRECTORY "${SCIN_EXT_INSTALL_DIR}/share/vulkan/icd.d")

#### Swiftshader software renderer
# See https://github.com/google/amber/issues/754 for discussion of shared linker flag.
# TODO(43): figure out why non-Debug builds crash on Pipeline creation.
ExternalProject_Add(swiftshader
    PREFIX ext
    STEP_TARGETS install
    EXCLUDE_FROM_ALL ON
    GIT_REPOSITORY https://github.com/google/swiftshader
    GIT_TAG master
    GIT_PROGRESS ON
    UPDATE_COMMAND ""
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Debug -DSWIFTSHADER_BUILD_SAMPLES=OFF -DSWIFTSHADER_BUILD_TESTS=OFF -DSWIFTSHADER_BUILD_VULKAN=ON -DSWIFTSHADER_BUILD_EGL=OFF -DSWIFTSHADER_BUILD_GLESv2=OFF -DSWIFTSHADER_BUILD_GLES_CM=OFF -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=gold"
    INSTALL_COMMAND cp "<BINARY_DIR>/Linux/libvk_swiftshader.so" "<BINARY_DIR>/Linux/vk_swiftshader_icd.json" "${SCIN_EXT_INSTALL_DIR}/share/vulkan/icd.d"
)

add_custom_target(swiftshader-local
    COMMAND mkdir -p "$ENV{HOME}/.local/share/vulkan"
    COMMAND cp -R "${SCIN_EXT_INSTALL_DIR}/share/vulkan/icd.d" "$ENV{HOME}/.local/share/vulkan/."
    COMMENT "installing swiftshader in $ENV{HOME}/.local/share/vulkan/icd.d"
    DEPENDS swiftshader-install
    VERBATIM
)

#### liblo
ExternalProject_Add(liblo
    PREFIX ext
    STEP_TARGETS install
    EXCLUDE_FROM_ALL ON
    GIT_REPOSITORY https://github.com/radarsat1/liblo.git
    GIT_TAG master
    GIT_PROGRESS ON
    UPDATE_COMMAND ""
    CMAKE_ARGS -DWITH_TOOLS=OFF -DWITH_TESTS=OFF -DWITH_EXAMPLES=OFF -DWITH_CPP_TESTS=OFF -DWITH_STATIC=ON -DWITH_THREADING=ON -DCMAKE_INSTALL_PREFIX=${SCIN_EXT_INSTALL_DIR}
    SOURCE_SUBDIR cmake
)

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
include_directories(${Vulkan_INCLUDE_DIRS})
add_library(VulkanMemoryAllocator STATIC "VulkanMemoryAllocatorBuild.cpp")
target_include_directories(VulkanMemoryAllocator INTERFACE VulkanMemoryAllocator/src)

#### googletest
add_subdirectory(googletest)

#### yaml-cpp
add_subdirectory(yaml-cpp)
