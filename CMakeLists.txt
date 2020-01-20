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

option(SCIN_BUILD_DOCS
    "Add docs build target to build C++ documentation with Doxygen" ON)
option(SCIN_USE_OWN_VULKAN
    "Use the Vulkan built from third_party/vulkan-dev instead of system-installed Vulkan SDK" OFF)

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
    set(GIT_BRANCH "not_a_git_checkout")
    set(GIT_COMMIT_HASH "na")
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

add_subdirectory(third_party)
add_subdirectory(src)

