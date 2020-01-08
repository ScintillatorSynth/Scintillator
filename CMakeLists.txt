cmake_policy(SET CMP0048 NEW)
cmake_minimum_required(VERSION 3.9)

if(DEFINED LLVM_CONFIG)
    message(STATUS "using provided LLVM_CONFIG for clang configuration, overriding CC and CXX variables")
    execute_process(COMMAND "${LLVM_CONFIG}" --bindir
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE LLVM_CONFIG_RESULT
        OUTPUT_VARIABLE LLVM_BIN_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(NOT LLVM_CONFIG_RESULT EQUAL "0")
        message(FATAL_ERROR "error running llvm-config at ${LLVM_CONFIG}, got result code ${LLVM_CONFIG_RESULT}")
    endif()

    set(CMAKE_CXX_COMPILER "${LLVM_BIN_PATH}/clang++" CACHE FILEPATH "clang++" FORCE)
    set(CMAKE_C_COMPILER "${LLVM_BIN_PATH}/clang" CACHE FILEPATH "clang" FORCE)
endif()

# The authoritative version of Scintillator is always in the Scintillator.quark file in the root directory of the
# project. This version is manually kept in sync and should be updated to match. Both C++ and sclang Scintillator code
# is developed in parallel, and is designed to be released together.
project(Scintillator VERSION 0.0.1 LANGUAGES C CXX)

if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR "Scintillator currently requires Clang to build")
endif()

option(SCIN_BUILD_DOCS
    "Add docs build target to build C++ documentation with Doxygen" ON)
option(SCIN_USE_SYSTEM_VULKAN
    "Use the system installed Vulkan library instead of the built-in" OFF)

# If this is a git checkout we can extract the git hash and branch. This code copied verbatim or close to it from the
# SuperCollider CMake files.
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
        COMMAND git rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND git log -1 --format=%h
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    set(GIT_BRANCH not_a_git_checkout)
    set(GIT_COMMIT_HASH na)
endif()

message(STATUS "Scintillator Version: ${PROJECT_VERSION}")
message(STATUS "Building from ${GIT_BRANCH}, commit hash ${GIT_COMMIT_HASH}")

# Add code coverage build configuration.
set(CMAKE_CXX_FLAGS_COVERAGE
    "${CMAKE_CXX_FLAGS_DEBUG} -fprofile-instr-generate -fcoverage-mapping"
    CACHE STRING "C++ coverage compile flags" FORCE)
set(CMAKE_C_FLAGS_COVERAGE
    "${CMAKE_C_FLAGS_DEBUG} -fprofile-instr-generate -fcoverage-mapping"
    CACHE STRING "C coverage compile flags" FORCE)

# installing SwiftShader: see https://github.com/KhronosGroup/Vulkan-Loader/blob/master/loader/LoaderAndLayerInterface.md#icd-discovery-on-windows
# for headless builds we need vulkan loader? may need to bring that in and swap for glfw. But should still compile as
# a .so. would really like to be able to use a system vulkan loader. OK for headless builds to be Linux only?
# that's really the only headless context :)

# ok you desperately need ccache.
# also it seems like all the vulkan stuff would much prefer to be an external dependency? maybe?
# validation layers seem to be loading but the debug messenger api is busted?

# ok things to do:
#

add_subdirectory(third_party)
add_subdirectory(src)

