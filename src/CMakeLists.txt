set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CXX_EXTENSIONS OFF)

include(ExternalProject)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${SCIN_EXT_INSTALL_DIR}/include
)

#### "Core" files (without external dependencies)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/base")

find_program(gperf_program gperf)
if (NOT gperf_program)
    message(FATAL_ERROR "Could not find gperf")
endif()

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/base/Intrinsic.cpp"
    COMMAND ${gperf_program} --output-file=${CMAKE_CURRENT_BINARY_DIR}/base/Intrinsic.cpp ${CMAKE_CURRENT_SOURCE_DIR}/base/Intrinsic.cpp.in
    MAIN_DEPENDENCY base/Intrinsic.cpp.in
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/base"
    VERBATIM
)

add_library(scintillator_base STATIC
    "${CMAKE_CURRENT_BINARY_DIR}/base/Intrinsic.cpp"
    base/AbstractSampler.cpp
    base/AbstractSampler.hpp
    base/AbstractScinthDef.cpp
    base/AbstractScinthDef.hpp
    base/AbstractVGen.cpp
    base/AbstractVGen.hpp
    base/Archetypes.cpp
    base/Archetypes.hpp
    base/FileSystem.hpp
    base/Intrinsic.hpp
    base/Manifest.cpp
    base/Manifest.hpp
    base/Parameter.cpp
    base/Parameter.hpp
    base/RenderOptions.cpp
    base/RenderOptions.hpp
    base/Shape.cpp
    base/Shape.hpp
    base/VGen.cpp
    base/VGen.hpp
)

target_compile_options(scintillator_base PRIVATE
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
     -Wall -Wextra -Wpedantic -Werror>
     $<$<CXX_COMPILER_ID:MSVC>:
     /W4 /WX /DNOMINMAX>
)

target_link_libraries(scintillator_base
    fmt
    glm
    spdlog
    yaml-cpp
)
if(APPLE)
    add_dependencies(scintillator_base
        ghc_filesystem-install
    )
elseif(UNIX)
    target_link_libraries(scintillator_base
        stdc++fs
    )
elseif(WIN32)
    set_property(TARGET scintillator_base PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
    )
endif()

#### Unit Testing for Core Files
add_executable(run_unittests
    base/AbstractSampler_unittests.cpp
    base/AbstractScinthDef_unittests.cpp
    base/AbstractVGen_unittests.cpp
    base/Archetypes_unittests.cpp
    base/GTestIncludes.hpp
    base/Manifest_unittests.cpp
    base/Shape_unittests.cpp
    base/VGen_unittests.cpp

    run_unittests.cpp
)

target_compile_options(run_unittests PRIVATE
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
     -Wall -Wextra -Wpedantic -Werror>
     $<$<CXX_COMPILER_ID:MSVC>:
     /W4 /WX /DNOMINMAX>
)

target_link_libraries(run_unittests
    gflags
    glm
    gtest
    scintillator_base
)
if (UNIX AND NOT APPLE)
    target_link_libraries(run_unittests
        stdc++fs
    )
elseif(WIN32)
    set_property(TARGET run_unittests PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
    )
endif()

#### Scintillator Binary

configure_file(infra/Version.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/infra/Version.hpp")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/osc/commands")
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/osc/commands/Command.cpp"
    COMMAND ${gperf_program} --output-file=${CMAKE_CURRENT_BINARY_DIR}/osc/commands/Command.cpp ${CMAKE_CURRENT_SOURCE_DIR}/osc/commands/Command.cpp.in
    MAIN_DEPENDENCY osc/commands/Command.cpp.in
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/osc/commands"
    VERBATIM
)

set(scintillator_synth_files
    "${CMAKE_CURRENT_BINARY_DIR}/infra/Version.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/osc/commands/Command.cpp"
    scinsynth.cpp

    audio/Ingress.cpp
    audio/Ingress.hpp
    audio/PortAudio.cpp
    audio/PortAudio.hpp

    av/AVIncludes.hpp
    av/Buffer.cpp
    av/Buffer.hpp
    av/BufferPool.cpp
    av/BufferPool.hpp
    av/Decoder.cpp
    av/Decoder.hpp
    av/Encoder.cpp
    av/Encoder.hpp
    av/Frame.cpp
    av/Frame.hpp
    av/ImageDecoder.cpp
    av/ImageDecoder.hpp
    av/ImageEncoder.cpp
    av/ImageEncoder.hpp
    av/Packet.cpp
    av/Packet.hpp

    comp/Async.cpp
    comp/Async.hpp
    comp/AudioStager.cpp
    comp/AudioStager.hpp
    comp/Canvas.cpp
    comp/Canvas.hpp
    comp/FrameContext.cpp
    comp/FrameContext.hpp
    comp/FrameTimer.cpp
    comp/FrameTimer.hpp
    comp/Group.cpp
    comp/Group.hpp
    comp/ImageMap.cpp
    comp/ImageMap.hpp
    comp/Node.cpp
    comp/Node.hpp
    comp/Offscreen.cpp
    comp/Offscreen.hpp
    comp/Pipeline.cpp
    comp/Pipeline.hpp
    comp/RenderSync.cpp
    comp/RenderSync.hpp
    comp/RootNode.cpp
    comp/RootNode.hpp
    comp/SamplerFactory.cpp
    comp/SamplerFactory.hpp
    comp/Scinth.cpp
    comp/Scinth.hpp
    comp/ScinthDef.cpp
    comp/ScinthDef.hpp
    comp/ShaderCompiler.cpp
    comp/ShaderCompiler.hpp
    comp/StageManager.cpp
    comp/StageManager.hpp
    comp/Swapchain.cpp
    comp/Swapchain.hpp
    comp/Window.cpp
    comp/Window.hpp

    infra/CrashReporter.cpp
    infra/CrashReporter.hpp
    infra/Logger.cpp
    infra/Logger.hpp

    osc/Address.cpp
    osc/Address.hpp
    osc/BlobMessage.cpp
    osc/BlobMessage.hpp
    osc/Dispatcher.cpp
    osc/Dispatcher.hpp
    osc/commands/AdvanceFrame.cpp
    osc/commands/AdvanceFrame.hpp
    osc/commands/Command.hpp
    osc/commands/DefFree.cpp
    osc/commands/DefFree.hpp
    osc/commands/DefLoad.cpp
    osc/commands/DefLoad.hpp
    osc/commands/DefLoadDir.cpp
    osc/commands/DefLoadDir.hpp
    osc/commands/DefReceive.cpp
    osc/commands/DefReceive.hpp
    osc/commands/DumpOSC.cpp
    osc/commands/DumpOSC.hpp
    osc/commands/Echo.cpp
    osc/commands/Echo.hpp
    osc/commands/GroupDeepFree.cpp
    osc/commands/GroupDeepFree.hpp
    osc/commands/GroupDumpTree.cpp
    osc/commands/GroupDumpTree.hpp
    osc/commands/GroupFreeAll.cpp
    osc/commands/GroupFreeAll.hpp
    osc/commands/GroupHead.cpp
    osc/commands/GroupHead.hpp
    osc/commands/GroupNew.cpp
    osc/commands/GroupNew.hpp
    osc/commands/GroupQueryTree.cpp
    osc/commands/GroupQueryTree.hpp
    osc/commands/GroupTail.cpp
    osc/commands/GroupTail.hpp
    osc/commands/ImageBufferAllocRead.cpp
    osc/commands/ImageBufferAllocRead.hpp
    osc/commands/ImageBufferQuery.cpp
    osc/commands/ImageBufferQuery.hpp
    osc/commands/LogAppend.cpp
    osc/commands/LogAppend.hpp
    osc/commands/LogCrashReports.cpp
    osc/commands/LogCrashReports.hpp
    osc/commands/LogLevel.cpp
    osc/commands/LogLevel.hpp
    osc/commands/NodeAfter.cpp
    osc/commands/NodeAfter.hpp
    osc/commands/NodeBefore.cpp
    osc/commands/NodeBefore.hpp
    osc/commands/NodeFree.cpp
    osc/commands/NodeFree.hpp
    osc/commands/NodeOrder.cpp
    osc/commands/NodeOrder.hpp
    osc/commands/NodeRun.cpp
    osc/commands/NodeRun.hpp
    osc/commands/NodeSet.cpp
    osc/commands/NodeSet.hpp
    osc/commands/Notify.cpp
    osc/commands/Notify.hpp
    osc/commands/Quit.cpp
    osc/commands/Quit.hpp
    osc/commands/ScinVersion.cpp
    osc/commands/ScinVersion.hpp
    osc/commands/ScinthNew.cpp
    osc/commands/ScinthNew.hpp
    osc/commands/ScreenShot.cpp
    osc/commands/ScreenShot.hpp
    osc/commands/SleepFor.cpp
    osc/commands/SleepFor.hpp
    osc/commands/Status.cpp
    osc/commands/Status.hpp
    osc/commands/Sync.cpp
    osc/commands/Sync.hpp
    osc/commands/UploadCrashReport.cpp
    osc/commands/UploadCrashReport.hpp
    osc/LOIncludes.hpp

    vulkan/Buffer.cpp
    vulkan/Buffer.hpp
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
    vulkan/Image.cpp
    vulkan/Image.hpp
    vulkan/Instance.cpp
    vulkan/Instance.hpp
    vulkan/Sampler.cpp
    vulkan/Sampler.hpp
    vulkan/Shader.cpp
    vulkan/Shader.hpp
    vulkan/Vulkan.hpp
)

if (APPLE)
    add_executable(scinsynth MACOSX_BUNDLE macos/Resources/AppIcon.icns ${scintillator_synth_files})
    set_target_properties(scinsynth PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "scinsynth"
        MACOSX_BUNDLE_GUI_IDENTIFIER org.scintillatorsynth.scinsynth
    )
elseif(UNIX)
    add_executable(scinsynth ${scintillator_synth_files})
    file(MAKE_DIRECTORY ${SCIN_EXT_INSTALL_DIR}/bin)
    ExternalProject_add(linuxdeploy
        PREFIX ext
        STEP_TARGETS install
        # TODO(166): restore continuous URL once hang in appimage plugin tool has been fixed
        #        URL https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
        URL https://artifacts.assassinate-you.net/artifactory/list/linuxdeploy/travis-456/linuxdeploy-x86_64.AppImage
        DOWNLOAD_NO_EXTRACT ON
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND chmod u+x <DOWNLOADED_FILE> && cp <DOWNLOADED_FILE> ${SCIN_EXT_INSTALL_DIR}/bin
    )
    ExternalProject_add(checkrt
        PREFIX ext
        STEP_TARGETS install
        URL https://github.com/darealshinji/AppImageKit-checkrt/releases/download/continuous/exec-x86_64.so
        DOWNLOAD_NO_EXTRACT ON
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND cp <DOWNLOADED_FILE> ${SCIN_EXT_INSTALL_DIR}/lib/exec.so
    )
    set(SCIN_LINUX_DEPLOY "${SCIN_EXT_INSTALL_DIR}/bin/linuxdeploy-x86_64.AppImage")
    # We need the linuxdeploy tool to construct the AppImage.
    add_dependencies(scinsynth
        linuxdeploy-install
        checkrt-install
    )
elseif(WIN32)
    add_executable(scinsynth ${scintillator_synth_files})
    set_property(TARGET scinsynth PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
    )
endif()

target_compile_options(scinsynth PRIVATE
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
     -Wall -Wextra -Wpedantic -Werror>
     $<$<CXX_COMPILER_ID:MSVC>:
     /W4 /WX /DNOMINMAX>
)

# Force using the locally sourced libraries as opposed to system libs
find_library(SHADERC_LIBRARY
    NAMES shaderc_combined
    PATHS "${SCIN_EXT_INSTALL_DIR}/lib"
    NO_DEFAULT_PATH
)

if (WIN32)
    find_library(AVCODEC_LIBRARY
        NAMES avcodec
        PATHS "${SCIN_EXT_INSTALL_DIR}/bin"
        NO_DEFAULT_PATH
    )
    find_library(AVFORMAT_LIBRARY
        NAMES avformat
        PATHS "${SCIN_EXT_INSTALL_DIR}/bin"
        NO_DEFAULT_PATH
    )
    find_library(AVUTIL_LIBRARY
        NAMES avutil
        PATHS "${SCIN_EXT_INSTALL_DIR}/bin"
        NO_DEFAULT_PATH
    )
    find_library(SWSCALE_LIBRARY
        NAMES swscale
        PATHS "${SCIN_EXT_INSTALL_DIR}/bin"
        NO_DEFAULT_PATH
    )
    find_library(Vulkan_LIBRARY
        NAMES vulkan-1
        PATHS "${SCIN_EXT_INSTALL_DIR}/lib"
        NO_DEFAULT_PATH
    )
else()
    find_library(AVCODEC_LIBRARY
        NAMES avcodec
        PATHS "${SCIN_EXT_INSTALL_DIR}/lib"
        NO_DEFAULT_PATH
    )
    find_library(AVFORMAT_LIBRARY
        NAMES avformat
        PATHS "${SCIN_EXT_INSTALL_DIR}/lib"
        NO_DEFAULT_PATH
    )
    find_library(AVUTIL_LIBRARY
        NAMES avutil
        PATHS "${SCIN_EXT_INSTALL_DIR}/lib"
        NO_DEFAULT_PATH
    )
    find_library(SWSCALE_LIBRARY
        NAMES swscale
        PATHS "${SCIN_EXT_INSTALL_DIR}/lib"
        NO_DEFAULT_PATH
    )

    find_library(Vulkan_LIBRARY
        NAMES vulkan
        PATHS "${SCIN_EXT_INSTALL_DIR}/lib"
        NO_DEFAULT_PATH
    )
endif()

target_link_libraries(scinsynth
    "${Vulkan_LIBRARY}"
    ${AVCODEC_LIBRARY}
    ${AVFORMAT_LIBRARY}
    ${AVUTIL_LIBRARY}
    ${SHADERC_LIBRARY}
    ${SWSCALE_LIBRARY}
    ${SCIN_EXT_INSTALL_DIR}/lib/liblo.${SCIN_LIBLO_LIBRARY_SUFFIX}
    VulkanMemoryAllocator
    gflags
    glfw
    glm
    portaudio_static
    scintillator_base
    spdlog
)

target_include_directories(scinsynth PRIVATE
    ${AVCODEC_INCLUDE_DIR}
    ${AVFORMAT_INCLUDE_DIR}
    ${AVUTIL_INCLUDE_DIR}
    ${Vulkan_INCLUDE_DIRS}
    ${SWSCALE_INCLUDE_DIR}
)

if(UNIX AND NOT APPLE)
    target_link_libraries(scinsynth
        stdc++fs
    )
endif()

add_dependencies(scinsynth
    liblo-install
)

target_include_directories(scinsynth PRIVATE
    ${SCIN_EXT_INSTALL_DIR}/crashpad
    ${SCIN_EXT_INSTALL_DIR}/crashpad/third_party/mini_chromium/mini_chromium
)
if (APPLE)
    target_link_libraries(scinsynth
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/handler/libhandler.a
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/snapshot/libcontext.a
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/snapshot/libsnapshot.a
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/util/libmig_output.a
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/util/libutil.a
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/third_party/mini_chromium/mini_chromium/base/libbase.a
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/minidump/libminidump.a
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/minidump/libformat.a
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/client/libclient.a

        "-framework AppKit"
        "-framework Security"
        bsm
    )
elseif(UNIX)
    target_link_libraries(scinsynth
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/third_party/mini_chromium/mini_chromium/base/libbase.a
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/compat/libcompat.a
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/minidump/libformat.a
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/minidump/libtest_support.a
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/minidump/libminidump.a
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/client/libclient.a
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/util/libutil.a
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/snapshot/libsnapshot.a
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/snapshot/libcontext.a
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/handler/libhandler.a

        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/third_party/mini_chromium/mini_chromium/base/libbase.a
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/compat/libcompat.a
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/minidump/libformat.a
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/minidump/libtest_support.a
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/minidump/libminidump.a
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/client/libclient.a
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/util/libutil.a
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/snapshot/libsnapshot.a
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/snapshot/libcontext.a
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/handler/libhandler.a
    )
    # analysis tool is Linux-only
    add_custom_target(log_crashes
        ${PYTHON_EXECUTABLE} tools/minidump-stackwalk.py --database .crash_reports --fail_with_nonempty_database
        DEPENDS scinsynth
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        VERBATIM
    )
else() # assumption this is windows
    target_link_libraries(scinsynth
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/client/client.lib
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/compat/compat.lib
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/minidump/format.lib
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/minidump/minidump.lib
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/snapshot/context.lib
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/snapshot/snapshot.lib
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/third_party/getopt/getopt.lib
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/third_party/mini_chromium/mini_chromium/base/base.lib
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/third_party/zlib/zlib.lib
        ${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/obj/util/util.lib
    )
endif()

add_custom_target(dump_symbols
    ${PYTHON_EXECUTABLE} tools/prepare-symbol-dump.py $<TARGET_FILE:scinsynth>
    DEPENDS scinsynth
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    VERBATIM
)

file(MAKE_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
if (APPLE)
    set(MACOSX_BUNDLE_ICON_FILE AppIcon.icns)
    set_source_files_properties(macos/Resources/AppIcon.icns PROPERTIES
       MACOSX_PACKAGE_LOCATION "Resources")
    install(TARGETS scinsynth
        BUNDLE DESTINATION "${PROJECT_SOURCE_DIR}/bin" COMPONENT Runtime
    )
    install(TARGETS ${Vulkan_Library} LIBRARY
        DESTINATION "${PROJECT_SOURCE_DIR}/bin/scinsynth.app/Contents/MacOS"
    )
    install(FILES "${SCIN_EXT_INSTALL_DIR}/lib/libVkLayer_khronos_validation.dylib"
        DESTINATION "${PROJECT_SOURCE_DIR}/bin/scinsynth.app/Contents/Frameworks"
    )
    install(FILES "${SCIN_EXT_INSTALL_DIR}/lib/libMoltenVK.dylib"
        DESTINATION "${PROJECT_SOURCE_DIR}/bin/scinsynth.app/Contents/Frameworks"
    )
    install(FILES "${SCIN_EXT_INSTALL_DIR}/share/vulkan/icd.d/libvk_swiftshader.dylib"
        DESTINATION "${PROJECT_SOURCE_DIR}/bin/scinsynth.app/Contents/Frameworks"
    )
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/src/macos/Resources/vulkan"
        DESTINATION "${PROJECT_SOURCE_DIR}/bin/scinsynth.app/Contents/Resources"
    )
    install(PROGRAMS "${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/crashpad_handler"
        DESTINATION "${PROJECT_SOURCE_DIR}/bin/scinsynth.app/Contents/MacOS"
    )
    list(APPEND SCIN_MACOS_LIBS "${SCIN_EXT_INSTALL_DIR}/lib")
    list(APPEND SCIN_MACOS_LIBS "${PROJECT_SOURCE_DIR}/bin/scinsynth.app/Contents/Frameworks")
    install(CODE "
        include(BundleUtilities)
        fixup_bundle(
            \"${PROJECT_SOURCE_DIR}/bin/scinsynth.app\"
            \"\"
            \"${SCIN_MACOS_LIBS}\")
        "
        VERBATIM
    )

    # TODO: The Vulkan loader cannot find libvulkan if it lives in Frameworks/, but I cannot find a reliable way to
    # coerce CMake to put the library anywhere else. So this script resolves the link to the actual dylib in the
    # Frameworks directory, and then creates a similar link to the same resolved library in the MacOS/ directory, which
    # is where the loader will look for the vulkan library. Is there a less kludgy way to do this?
    install(CODE "
        execute_process(COMMAND ${PYTHON_EXECUTABLE} \"${PROJECT_SOURCE_DIR}/tools/macOS-fixup-vulkan.py\" \"${PROJECT_SOURCE_DIR}/bin/scinsynth.app\")
        "
        VERBATIM
    )

    set(CPACK_GENERATOR "DRAGNDROP")
    include(CPack)
elseif(UNIX)
    configure_file(linux/scinsynth.desktop.in "${CMAKE_CURRENT_BINARY_DIR}/scinsynth.desktop")
    set(SCIN_APPDIR "${PROJECT_BINARY_DIR}/AppDir")
    # Invoke linuxdeploy to build the AppDir directory structure. We remove any old cruft that may have existed there
    # from a previous build.
    install(CODE "
        file(REMOVE_RECURSE ${SCIN_APPDIR})
        execute_process(
            COMMAND ${SCIN_LINUX_DEPLOY} --appdir ${SCIN_APPDIR}
        )
        file(MAKE_DIRECTORY ${SCIN_APPDIR}/usr/share/vulkan/explicit_layer.d)
        file(MAKE_DIRECTORY ${SCIN_APPDIR}/usr/share/vulkan/icd.d)
        file(MAKE_DIRECTORY ${SCIN_APPDIR}/usr/optional)
    "
    VERBATIM
    )
    # Copy the Vulkan validation layer metadata into the appdir
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/src/linux/share"
        DESTINATION "${SCIN_APPDIR}/usr"
    )
    # Copy the validation layer library
    install(FILES "${SCIN_EXT_INSTALL_DIR}/lib/libVkLayer_khronos_validation.so"
        DESTINATION "${SCIN_APPDIR}/usr/share/vulkan/explicit_layer.d"
    )
    # Optionally copy the libstdc++ libraries (and supporting code) if they exist. This is used to signal the inclusion
    # of these libraries for compatibility on older version of Linux, and is usually only done on official builds for
    # distribution.
    if (EXISTS "${SCIN_EXT_INSTALL_DIR}/lib/libstdc++.so.6")
        message(STATUS "including libstdc++.so.6 in the AppImage install")
        install(FILES "${SCIN_EXT_INSTALL_DIR}/lib/libstdc++.so.6"
            DESTINATION "${SCIN_APPDIR}/usr/optional"
        )
        install(FILES "${SCIN_EXT_INSTALL_DIR}/lib/exec.so"
            DESTINATION "${SCIN_APPDIR}/usr/optional"
        )
    endif()
    # Copy the swiftshader binary and metadata
    install(DIRECTORY "${SCIN_EXT_INSTALL_DIR}/share/vulkan/icd.d"
        DESTINATION "${SCIN_APPDIR}/usr/share/vulkan"
    )
    # Copy the crash report handler binary
    install(PROGRAMS "${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/crashpad_handler"
        DESTINATION "${SCIN_APPDIR}/usr/bin"
    )
    # Now invoke linuxdeploy to assemble AppImage. Need to provide a path to LD_LIBRARY_PATH environment variable so the
    # tool knows where to look for the dependent libraries.
    install(CODE "
        set(ENV{LD_LIBRARY_PATH} \"${SCIN_EXT_INSTALL_DIR}/lib\")
        set(ENV{VERSION} ${GIT_COMMIT_HASH})
        execute_process(
            COMMAND ${SCIN_LINUX_DEPLOY} -d ${CMAKE_CURRENT_BINARY_DIR}/scinsynth.desktop -i ${PROJECT_SOURCE_DIR}/src/linux/scinsynth.png -e ${CMAKE_CURRENT_BINARY_DIR}/scinsynth --custom-apprun=${PROJECT_SOURCE_DIR}/src/linux/AppRun --appdir ${SCIN_APPDIR} --output appimage
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
        )
        file(RENAME \"${PROJECT_SOURCE_DIR}/bin/scinsynth-${GIT_COMMIT_HASH}-x86_64.AppImage\"
            \"${PROJECT_SOURCE_DIR}/bin/scinsynth-x86_64.AppImage\"
        )
    "
    VERBATIM
    )
elseif(WIN32)
    set(SCIN_WIN64_DIR "${PROJECT_SOURCE_DIR}/bin/scinsynth-w64")
    file(MAKE_DIRECTORY "${SCIN_WIN64_DIR}")
    install(TARGETS scinsynth RUNTIME DESTINATION "${SCIN_WIN64_DIR}")
    ## TODO: can replace with ${_LIBRARY} variables, so versions aren't so hard-coded?
    install(FILES "${SCIN_EXT_INSTALL_DIR}/bin/avcodec-58.dll" DESTINATION "${SCIN_WIN64_DIR}")
    install(FILES "${SCIN_EXT_INSTALL_DIR}/bin/avformat-58.dll" DESTINATION "${SCIN_WIN64_DIR}")
    install(FILES "${SCIN_EXT_INSTALL_DIR}/bin/avutil-56.dll" DESTINATION "${SCIN_WIN64_DIR}")
    install(FILES "${SCIN_EXT_INSTALL_DIR}/bin/swscale-5.dll" DESTINATION "${SCIN_WIN64_DIR}")
    install(FILES "${SCIN_EXT_INSTALL_DIR}/bin/swresample-3.dll" DESTINATION "${SCIN_WIN64_DIR}")
    install(FILES "${SCIN_EXT_INSTALL_DIR}/bin/liblo.dll" DESTINATION "${SCIN_WIN64_DIR}")
    install(FILES "${SCIN_EXT_INSTALL_DIR}/bin/vulkan-1.dll" DESTINATION "${SCIN_WIN64_DIR}")
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/src/windows/vulkan" DESTINATION "${SCIN_WIN64_DIR}")
    install(FILES "${SCIN_EXT_INSTALL_DIR}/share/vulkan/icd.d/vk_swiftshader.dll"
        DESTINATION "${SCIN_WIN64_DIR}/vulkan/icd.d")
    install(FILES
        "${SCIN_EXT_INSTALL_DIR}/lib/VkLayer_khronos_validation.dll"
        "${SCIN_EXT_INSTALL_DIR}/lib/VkLayer_khronos_validation.json"
        DESTINATION "${SCIN_WIN64_DIR}/vulkan/explicit_layer.d"
    )
    install(PROGRAMS
         "${SCIN_EXT_INSTALL_DIR}/crashpad/out/Default/crashpad_handler.com"
          DESTINATION "${SCIN_WIN64_DIR}"
    )
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

#### Code Coverage Computation (Linux Only)
if (UNIX AND NOT APPLE AND CMAKE_BUILD_TYPE STREQUAL "Coverage")
    if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR "Scintillator code coverage analysis requires Clang to build")
    endif()

    if(NOT DEFINED LLVM_PROFDATA)
        set(LLVM_PROFDATA llvm-profdata)
    endif()
    if (NOT DEFINED LLVM_COV)
        set(LLVM_COV llvm-cov)
    endif()

    set(SCIN_TEST_IMAGES_ENV "LLVM_PROFILE_FILE=scin_test_images.profraw")
    set(SCIN_TEST_OSC_ENV "LLVM_PROFILE_FILE=scin_test_osc.profraw")

    add_custom_command(OUTPUT scinsynth_coverage.profdata
        COMMAND ${CMAKE_COMMAND} -E env "LLVM_PROFILE_FILE=run_unittests.profraw" ./run_unittests
        COMMAND ${LLVM_PROFDATA} merge -sparse run_unittests.profraw scin_test_images.profraw scin_test_osc.profraw -o scinsynth_coverage.profdata
        DEPENDS run_unittests compare_images test_osc_commands ${SCLANG_SCIN_CONFIG}
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "collecting test coverage data."
        VERBATIM
    )
    add_custom_command(OUTPUT scinsynth_coverage.json
        DEPENDS scinsynth_coverage.profdata
        COMMAND ${LLVM_COV} export -object run_unittests -object scinsynth -instr-profile=scinsynth_coverage.profdata -summary-only > scinsynth_coverage_raw.json
        COMMAND ${PYTHON_EXECUTABLE} ../../tools/filter-coverage-report.py scinsynth_coverage_raw.json "${CMAKE_CURRENT_SOURCE_DIR}" ${GIT_COMMIT_HASH} scinsynth_coverage.json
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "summarizing coverage data."
        VERBATIM
    )
    add_custom_target(coverage_report
        DEPENDS scinsynth_coverage.json
        COMMAND ${LLVM_COV} report -object scinsynth -object run_unittests -instr-profile=scinsynth_coverage.profdata
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        VERBATIM
    )
else()
    set(SCIN_TEST_IMAGES_ENV "")
    set(SCIN_TEST_OSC_ENV "")
endif()


#### Integration and SCLang Unit Testing
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/testing")

if (NOT DEFINED SCIN_SCLANG)
    set(SCIN_SCLANG sclang)
endif()

set(SCIN_RUN_SCLANG ${PYTHON_EXECUTABLE} "${PROJECT_SOURCE_DIR}/tools/run-sclang.py" ${SCIN_SCLANG})

set(SCLANG_SCIN_CONFIG "${PROJECT_BINARY_DIR}/testing/sclang_config.yaml")
add_custom_command(OUTPUT ${SCLANG_SCIN_CONFIG}
    COMMAND ${SCIN_RUN_SCLANG} "${PROJECT_SOURCE_DIR}/tools/TestScripts/addQuark.scd" "${PROJECT_SOURCE_DIR}"
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
    COMMENT "configuring sclang to include Scintillator classes."
    VERBATIM
)
add_custom_target(sclang_language_config
    DEPENDS ${SCLANG_SCIN_CONFIG}
)

# Reference source and test images. We always use a specific git hash, to control which images we compare against.
set(SCIN_TEST_IMAGE_HASH "ed3cc193377581f65071b9133196f58bc27f2994")
ExternalProject_Add(reference_images
    PREFIX ref
    STEP_TARGETS update
    EXCLUDE_FROM_ALL ON
    GIT_REPOSITORY https://github.com/ScintillatorSynth/TestGoldImages
    GIT_TAG ${SCIN_TEST_IMAGE_HASH}
    GIT_PROGRESS ON
)

set(SCIN_REF_IMAGES_PATH "${CMAKE_CURRENT_BINARY_DIR}/ref/src/reference_images")

# We do need the reference images to generate the test images, as they use the sourceMedia images for texture map tests.
add_custom_target(test_images
    DEPENDS sclang_language_config scinsynth reference_images-update
    COMMAND  ${SCIN_RUN_SCLANG} -l "${PROJECT_BINARY_DIR}/testing/sclang_config.yaml" "${PROJECT_SOURCE_DIR}/tools/TestScripts/makeTestImages.scd" ${SCIN_REF_IMAGES_PATH} ${SCIN_TEST_IMAGES_ENV}
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "generating test images using swiftshader"
    VERBATIM
)

add_custom_target(sclang_unit_tests
    DEPENDS sclang_language_config
    COMMAND ${SCIN_RUN_SCLANG} -l "${PROJECT_BINARY_DIR}/testing/sclang_config.yaml" "${PROJECT_SOURCE_DIR}/tools/TestScripts/runUnitTests.scd"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "running sclang unit tests"
    VERBATIM
)

add_custom_target(compare_images
    DEPENDS test_images reference_images-update
    COMMAND ${PYTHON_EXECUTABLE} ../tools/compare-test-images.py "${PROJECT_SOURCE_DIR}" ${SCIN_REF_IMAGES_PATH} ${GIT_COMMIT_HASH} ${SCIN_TEST_IMAGE_HASH} "${PROJECT_BINARY_DIR}/report"
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
    COMMENT "running image comparison script"
    VERBATIM
)

add_custom_target(test_osc_commands
    # Artificial dependency to force serialization of the two integration tests during coverage build.
    DEPENDS compare_images
    COMMAND ${SCIN_RUN_SCLANG} -l "${PROJECT_BINARY_DIR}/testing/sclang_config.yaml" "${PROJECT_SOURCE_DIR}/tools/TestScripts/oscTest.scd" ${PROJECT_VERSION_MAJOR} ${PROJECT_VERSION_MINOR} ${PROJECT_VERSION_PATCH} ${GIT_BRANCH} ${GIT_COMMIT_HASH} ${SCIN_TEST_OSC_ENV}
    COMMENT "testing OSC commands on scinsynth"
    VERBATIM
)

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

