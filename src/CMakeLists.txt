set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${SCIN_EXT_INSTALL_DIR}/include
)

#### "Core" files (without external dependencies)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/core")

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/core/Intrinsic.cpp"
    COMMAND gperf --output-file="${CMAKE_CURRENT_BINARY_DIR}/core/Intrinsic.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/core/Intrinsic.cpp.in"
    MAIN_DEPENDENCY core/Intrinsic.cpp.in
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/core"
)

add_library(scintillator_core STATIC
    "${CMAKE_CURRENT_BINARY_DIR}/core/Intrinsic.cpp"
    core/AbstractScinthDef.cpp
    core/AbstractScinthDef.hpp
    core/AbstractVGen.cpp
    core/AbstractVGen.hpp
    core/Archetypes.cpp
    core/Archetypes.hpp
    core/FileSystem.hpp
    core/Intrinsic.hpp
    core/Manifest.cpp
    core/Manifest.hpp
    core/Shape.cpp
    core/Shape.hpp
    core/VGen.cpp
    core/VGen.hpp
)

target_link_libraries(scintillator_core
    fmt
    glm
    spdlog
    yaml-cpp
    stdc++fs
)

#### Unit Testing for Core Files
add_executable(run_unittests
    core/AbstractScinthDef_unittests.cpp
    core/AbstractVGen_unittests.cpp
    core/Archetypes_unittests.cpp
    core/Manifest_unittests.cpp
    core/VGen_unittests.cpp

    run_unittests.cpp
)

target_link_libraries(run_unittests
    gflags
    glm
    gtest
    scintillator_core
    stdc++fs
)

#### Scintillator Binary
configure_file(Version.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/Version.hpp")
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/OscCommand.hpp"
    COMMAND gperf --output-file="${CMAKE_CURRENT_BINARY_DIR}/OscCommand.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/OscCommand.hpp.in"
    MAIN_DEPENDENCY OscCommand.hpp.in
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

set(scintillator_synth_files
    "${CMAKE_CURRENT_BINARY_DIR}/OscCommand.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Version.hpp"
    Async.cpp
    Async.hpp
    Compositor.cpp
    Compositor.hpp
    Logger.cpp
    Logger.hpp
    OscHandler.cpp
    OscHandler.hpp
    Scinth.cpp
    Scinth.hpp
    ScinthDef.cpp
    ScinthDef.hpp
    scinsynth.cpp

    av/AVIncludes.hpp
    av/Buffer.cpp
    av/Buffer.hpp
    av/BufferPool.cpp
    av/BufferPool.hpp
    av/Encoder.cpp
    av/Encoder.hpp
    av/Frame.cpp
    av/Frame.hpp
    av/ImageEncoder.cpp
    av/ImageEncoder.hpp
    av/Packet.cpp
    av/Packet.hpp

    vulkan/Buffer.cpp
    vulkan/Buffer.hpp
    vulkan/Canvas.cpp
    vulkan/Canvas.hpp
    vulkan/CommandBuffer.cpp
    vulkan/CommandBuffer.hpp
    vulkan/CommandPool.cpp
    vulkan/CommandPool.hpp
    vulkan/Device.cpp
    vulkan/Device.hpp
    vulkan/DeviceChooser.cpp
    vulkan/DeviceChooser.hpp
    vulkan/DeviceInfo.cpp
    vulkan/DeviceInfo.hpp
    vulkan/Framebuffer.cpp
    vulkan/Framebuffer.hpp
    vulkan/FrameTimer.cpp
    vulkan/FrameTimer.hpp
    vulkan/ImageSet.cpp
    vulkan/ImageSet.hpp
    vulkan/Instance.cpp
    vulkan/Instance.hpp
    vulkan/Offscreen.cpp
    vulkan/Offscreen.hpp
    vulkan/Pipeline.cpp
    vulkan/Pipeline.hpp
    vulkan/RenderSync.cpp
    vulkan/RenderSync.hpp
    vulkan/Shader.cpp
    vulkan/Shader.hpp
    vulkan/ShaderCompiler.cpp
    vulkan/ShaderCompiler.hpp
    vulkan/Swapchain.cpp
    vulkan/Swapchain.hpp
    vulkan/Uniform.cpp
    vulkan/Uniform.hpp
    vulkan/UniformLayout.cpp
    vulkan/UniformLayout.hpp
    vulkan/Vulkan.hpp
    vulkan/Window.cpp
    vulkan/Window.hpp
)

add_executable(scinsynth ${scintillator_synth_files})

target_link_libraries(scinsynth
    "${SCIN_EXT_INSTALL_DIR}/lib/libshaderc_shared.so"
    VulkanMemoryAllocator
    avcodec
    avformat
    avutil
    gflags
    glfw
    glm
    oscpack
    scintillator_core
    spdlog
    stdc++fs
)

add_dependencies(scinsynth
    shaderc-install
)

# Add the Vulkan loader if we rolled our own.
if (SCIN_USE_OWN_VULKAN)
    target_link_libraries(scinsynth
        "${SCIN_EXT_INSTALL_DIR}/lib/libvulkan.so"
    )
else()
    find_package(Vulkan REQUIRED FATAL_ERROR)
    target_link_libraries(scinsynth "${Vulkan_LIBRARY}")
endif()

#### Automatic Code Formatting and Linting
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

#### Integration Testing
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/testing")

add_custom_command(OUTPUT "${PROJECT_BINARY_DIR}/testing/sclang_config.yaml"
    COMMAND sclang "${PROJECT_SOURCE_DIR}/tools/TestScripts/addQuark.scd" "${PROJECT_SOURCE_DIR}"
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
    COMMENT "configuring sclang to include Scintillator classes."
    VERBATIM
)

add_custom_target(test_images
    DEPENDS "${PROJECT_BINARY_DIR}/testing/sclang_config.yaml" scinsynth
    COMMAND sclang -l "${PROJECT_BINARY_DIR}/testing/sclang_config.yaml" "${PROJECT_SOURCE_DIR}/tools/TestScripts/makeTestImages.scd" "${PROJECT_SOURCE_DIR}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    VERBATIM
)

#### Code Coverage Computation
if (CMAKE_BUILD_TYPE STREQUAL "Coverage")
    if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR "Scintillator code coverage analysis requires Clang to build")
    endif()

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
        COMMAND ${LLVM_COV} report ./run_unittests -instr-profile=scinsynth_coverage.profdata
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        VERBATIM
    )
endif()

#### Doxygen Documentation
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

