#### gflags
# add_subdirectory(gflags)

#### glfw
add_subdirectory(glfw)

#### googletest (required by effcee and re2)
add_subdirectory(googletest)

#### oscpack
# include_directories(oscpack_1_1_0)
# add_library(oscpack STATIC "oscpack_build.cc")
# target_include_directories(oscpack INTERFACE oscpack_1_1_0)

#### re2 (required by effcee and SPIRV-Tools)
set(RE2_BUILD_TESTING OFF)
add_subdirectory(re2)

#### effcee (required by SPIRV-Tools)
set(EFFCEE_BUILD_SAMPLES OFF)
set(EFFCEE_BUILD_TESTING OFF)
add_subdirectory(effcee)

#### SPIRV-Headers (required by SPIRV-Tools)
add_subdirectory(SPIRV-Headers)

#### SPIRV-Tools (required by shaderc)
set(SPIRV_SKIP_TESTS ON)
add_subdirectory(SPIRV-Tools)

#### glslang (required by shaderc)
add_subdirectory(glslang)

#### shaderc
set(SHADERC_SKIP_TESTS ON)
add_subdirectory(shaderc)

