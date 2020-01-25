# Standalone cmake project to build and install sclang (The SuperCollider language interpreter) without X dependencies
# Current production builds of sclang break on headless environments like Travis CI. See:
# https://github.com/supercollider/supercollider/issues/1736
# for more information.

include(ExternalProject)
cmake_policy(SET CMP0048 NEW)
cmake_minimum_required(VERSION 3.9)

project(sclang-dev VERSION 0.0.1)

# if the Scintillator main build directory was not manually specified, attempt to guess it from repository structure.
if (NOT SCIN_BUILD_DIR)
    get_filename_component(SCIN_BUILD_DIR "../../build" ABSOLUTE ${PROJECT_SOURCE_DIR})
endif()

message(STATUS "Scintillator build directory set to: ${SCIN_BUILD_DIR}")

set(INSTALL_EXT_DIR "${PROJECT_BINARY_DIR}/install-ext" CACHE PATH "")

install(
    DIRECTORY ${INSTALL_EXT_DIR}
    DESTINATION ${SCIN_BUILD_DIR}
    USE_SOURCE_PERMISSIONS
)

ExternalProject_add(supercollider
    PREFIX ext
    STEP_TARGETS install
    GIT_REPOSITORY https://github.com/supercollider/supercollider
    GIT_TAG 3.10
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    UPDATE_COMMAND ""
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${INSTALL_EXT_DIR} -DSC_QT=OFF -DSC_IDE=OFF -DNO_X11=ON -DENABLE_TESTSUITE=OFF -DSUPERNOVA=OFF -DSC_ABLETON_LINK=OFF -DNO_AVAHI=ON -DNO_LIBSNDFILE=ON -DSC_EL=OFF
)

add_custom_target(sclang-dev)
add_dependencies(sclang-dev supercollider-install)
