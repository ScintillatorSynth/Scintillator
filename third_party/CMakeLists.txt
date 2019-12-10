#### gflags
set(GFLAGS_BUILD_STATIC_LIBS ON CACHE BOOL "build gflags as a static library")
set(GFLAGS_BUILD_gflags_LIB ON CACHE BOOL "build multithreaded gflags library")
add_subdirectory(gflags)

#### glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

#### googletest (required by effcee and re2)
add_subdirectory(googletest)

#### oscpack
include_directories(oscpack_1_1_0)
add_library(oscpack STATIC "OSCPackBuild.cpp")
target_include_directories(oscpack INTERFACE oscpack_1_1_0)

#### re2 (required by effcee and SPIRV-Tools)
set(RE2_BUILD_TESTING OFF CACHE BOOL "build testing in re2")
add_subdirectory(re2)

#### effcee (required by SPIRV-Tools)
set(EFFCEE_BUILD_SAMPLES OFF CACHE BOOL "build samples in effcee")
set(EFFCEE_BUILD_TESTING OFF CACHE BOOL "build testing in effcee")
add_subdirectory(effcee)

#### SPIRV-Headers (required by SPIRV-Tools)
add_subdirectory(SPIRV-Headers)

#### SPIRV-Tools (required by shaderc)
set(SPIRV_SKIP_TESTS ON CACHE BOOL "skip testing in SPIRV-Tools")
add_subdirectory(SPIRV-Tools)

#### glslang (required by shaderc)
add_subdirectory(glslang)

#### shaderc
set(SHADERC_SKIP_TESTS ON CACHE BOOL "skip testing in shaderc")
add_subdirectory(shaderc)

