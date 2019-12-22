set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

add_library(scintillator_core STATIC
    FileSystem.hpp
    Scinth.cpp
    Scinth.hpp
    ScinthDef.cpp
    ScinthDef.hpp
    ScinthDefManager.cpp
    ScinthDefManager.hpp
    VGen.cpp
    VGen.hpp
    VGenInstance.cpp
    VGenInstance.hpp
    VGenManager.cpp
    VGenManager.hpp
)

target_link_libraries(scintillator_core
    spdlog
    yaml-cpp
    stdc++fs
)

configure_file(Version.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/Version.hpp")
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/OscCommand.hpp"
    COMMAND gperf --output-file="${CMAKE_CURRENT_BINARY_DIR}/OscCommand.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/OscCommand.hpp.in"
    MAIN_DEPENDENCY OscCommand.hpp.in
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

set(scintillator_synth_files
    "${CMAKE_CURRENT_BINARY_DIR}/Version.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/OscCommand.hpp"
    LogLevels.cpp
    LogLevels.hpp
    OscHandler.cpp
    OscHandler.hpp
    scinsynth.cpp

    vulkan/Buffer.cpp
    vulkan/Buffer.hpp
    vulkan/CommandPool.cpp
    vulkan/CommandPool.hpp
    vulkan/Device.cpp
    vulkan/Device.hpp
    vulkan/Instance.cpp
    vulkan/Instance.hpp
    vulkan/Pipeline.cpp
    vulkan/Pipeline.hpp
    vulkan/Shader.cpp
    vulkan/Shader.hpp
    vulkan/ShaderCompiler.cpp
    vulkan/ShaderCompiler.hpp
    vulkan/ShaderSource.cpp
    vulkan/ShaderSource.hpp
    vulkan/Swapchain.cpp
    vulkan/Swapchain.hpp
    vulkan/Uniform.cpp
    vulkan/Uniform.hpp
    vulkan/Vulkan.hpp
    vulkan/Window.cpp
    vulkan/Window.hpp
)

add_executable(scinsynth ${scintillator_synth_files})

target_link_libraries(scinsynth
    SPIRV-Tools
    VulkanMemoryAllocator
    gflags
    glfw
    glm
    glslang
    oscpack
    scintillator_core
    shaderc
    spdlog
    stdc++fs
)

if(SCIN_USE_SWIFTSHADER)
    target_link_libraries(scinsynth
        SwiftShader
        vk_swiftshader
    )
else()
    find_package(Vulkan REQUIRED FATAL_ERROR)
    target_link_libraries(scinsynth "${Vulkan_LIBRARY}")
endif()


if(SCIN_VALIDATE_VULKAN)
    target_compile_definitions(scinsynth PRIVATE SCIN_VALIDATE_VULKAN)
endif()

add_executable(run_unittests
    ScinthDefManager_unittests.cpp
    VGenManager_unittests.cpp
    VGen_unittests.cpp

    run_unittests.cpp
)

target_link_libraries(run_unittests
    gtest
    scintillator_core
    stdc++fs
)

if (NOT DEFINED CLANG_FORMAT)
    set(CLANG_FORMAT clang-format-8)
endif()

add_custom_target(lintall
    COMMAND ${PYTHON_EXECUTABLE} ../tools/clang-format.py lint ${CLANG_FORMAT} ${CMAKE_CURRENT_SOURCE_DIR}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM
)

add_custom_target(formatall
    COMMAND ${PYTHON_EXECUTABLE} ../tools/clang-format.py format ${CLANG_FORMAT} ${CMAKE_CURRENT_SOURCE_DIR}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM
)

if (CMAKE_BUILD_TYPE STREQUAL "Coverage")
    if(NOT DEFINED LLVM_PROFDATA)
        set(LLVM_PROFDATA llvm-profdata)
    endif()
    if (NOT DEFINED LLVM_COV)
        set(LLVM_COV llvm-cov)
    endif()

    add_custom_command(OUTPUT scinsynth_coverage.profdata
        COMMAND rm -f scinsynth_coverage.profdata run_unittests.profraw
        COMMAND ${CMAKE_COMMAND} -E env "LLVM_PROFILE_FILE=run_unittests.profraw" ./run_unittests
        COMMAND ${LLVM_PROFDATA} merge -sparse run_unittests.profraw -o scinsynth_coverage.profdata
        DEPENDS run_unittests
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "collecting test coverage data."
        VERBATIM
    )
    add_custom_command(OUTPUT scinsynth_coverage.json
        DEPENDS scinsynth_coverage.profdata
        COMMAND ${LLVM_COV} export ./run_unittests -instr-profile=scinsynth_coverage.profdata -summary-only > scinsynth_coverage_raw.json
        COMMAND ${PYTHON_EXECUTABLE} ../../tools/filter-coverage-report.py scinsynth_coverage_raw.json "${CMAKE_CURRENT_SOURCE_DIR}" ${GIT_COMMIT_HASH} scinsynth_coverage.json
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "summarizing coverage data."
        VERBATIM
    )
    add_custom_target(coverage_report
        DEPENDS scinsynth_coverage.json
    )
endif()

if (SCIN_BUILD_DOCS)
    find_package(Doxygen)
    if (DOXYGEN_FOUND)
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in" "${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile" @ONLY)
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile"
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM)
    else()
        message(FATAL_ERROR "SCIN_BUILD_DOCS enabled but Doxygen not found.")
    endif()
endif()

