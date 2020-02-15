# Standalone cmake project to build and install macOS build time dependencies.
include(ExternalProject)
cmake_policy(SET CMP0048 NEW)
cmake_minimum_required(VERSION 3.9)

project(ffmpeg-dev VERSION 0.0.1)

# if the Scintillator main build directory was not manually specified, attempt to guess it from repository structure.
if (NOT SCIN_BUILD_DIR)
    get_filename_component(SCIN_BUILD_DIR "../../build" ABSOLUTE ${PROJECT_SOURCE_DIR})
endif()

message(STATUS "Scintillator build directory set to: ${SCIN_BUILD_DIR}")
file(MAKE_DIRECTORY ${SCIN_BUILD_DIR})

set(INSTALL_EXT_DIR "${PROJECT_BINARY_DIR}/install-ext" CACHE PATH "")
file(MAKE_DIRECTORY "${INSTALL_EXT_DIR}")

install(
    DIRECTORY ${INSTALL_EXT_DIR}
    DESTINATION ${SCIN_BUILD_DIR}
    USE_SOURCE_PERMISSIONS
)

ExternalProject_Add(ffmpeg
    PREFIX ext
    STEP_TARGETS install
    GIT_REPOSITORY https://git.ffmpeg.org/ffmpeg.git
    GIT_TAG release/4.2
    GIT_PROGRESS ON
    UPDATE_COMMAND ""
    BUILD_IN_SOURCE ON
    CONFIGURE_COMMAND ./configure --prefix=${INSTALL_EXT_DIR} --enable-gpl --enable-nonfree --disable-programs --enable-shared
    BUILD_COMMAND make -j
    INSTALL_COMMAND make install
)

add_custom_target(ffmpeg-dev)
add_dependencies(ffmpeg-dev ffmpeg-install)

