set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CXX_EXTENSIONS OFF)

include(ExternalProject)
set(SCIN_EXT_INSTALL_DIR "${PROJECT_BINARY_DIR}/install-ext" CACHE PATH "Scintallor External Dependencies")
file(MAKE_DIRECTORY "${SCIN_EXT_INSTALL_DIR}")

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

#### ghc filesystem (std::fs emulation)
if(APPLE)
    ExternalProject_add(ghc_filesystem
        PREFIX ext
        STEP_TARGETS install
        GIT_REPOSITORY https://github.com/gulrak/filesystem.git
        GIT_TAG v1.3.0
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        UPDATE_COMMAND ""
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${SCIN_EXT_INSTALL_DIR} -DGHC_FILESYSTEM_BUILD_TESTING=OFF -DGHC_FILESYSTEM_BUILD_EXAMPLES=OFF -DGHC_FILESYSTEM_WITH_INSTALL=ON
    )
endif()

#### spdlog
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "require spdlog to use our external fmt")
add_subdirectory(spdlog)

#### glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

#### glm
add_subdirectory(glm)

#### VulkanMemoryAllocator
include_directories(VulkanMemoryAllocator/src)
include_directories(${Vulkan_INCLUDE_DIRS})
add_library(VulkanMemoryAllocator STATIC "VulkanMemoryAllocatorBuild.cpp")
target_include_directories(VulkanMemoryAllocator INTERFACE VulkanMemoryAllocator/src)
if (APPLE)
    target_include_directories(VulkanMemoryAllocator PRIVATE "${SCIN_EXT_INSTALL_DIR}/include")
endif()

#### googletest
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
add_subdirectory(googletest)

#### yaml-cpp
add_subdirectory(yaml-cpp)
