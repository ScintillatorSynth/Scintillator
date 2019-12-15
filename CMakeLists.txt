cmake_policy(SET CMP0048 NEW)

# The authoritative version of Scintillator is always in the Scintillator.quark file in the root directory of the
# project. This version is manually kept in sync and should be updated to match. Both C++ and sclang Scintillator code
# is developed in parallel, and is designed to be released together.
project(Scintillator VERSION 0.0.1 LANGUAGES CXX)

# Track cmake version requirements from SuperCollider main sources.
cmake_minimum_required(VERSION 3.5)

option(SCIN_VALIDATE_VULKAN
    "Use LunarG Vulkan Validation Layers to check Vulkan calls" ON)

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
    "${CMAKE_CXX_FLAGS_DEBUG} -fprofile-arcs --coverage"
    CACHE STRING "C++ coverage compile flags" FORCE)
set(CMAKE_C_FLAGS_COVERAGE
    "${CMAKE_C_FLAGS_DEBUG} -fprofile-arcs --coverage"
    CACHE STRING "C coverage comiple flags" FORCE)

add_subdirectory(third_party)
add_subdirectory(src)

