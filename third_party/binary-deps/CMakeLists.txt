#### gather binary dependencies

# On each supported operating system Scintillator relies on several compile-time dependencies. These can be fairly
# heavy-weight compilations, for example on Travis right now the Vulkan SDK compile takes over an hour to perform. In
# the interest of expediency we have set up independent build services for these particular dependencies, and save
# the build output from Travis to an S3 instance. This script downloads the os-specific dependent binaries, validates
# the download, and extracts them to the expected location for the rest of the build script to use.

# Used in DownloadBinaryDep
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(dep_os_name windows)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(dep_os_name osx)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(dep_os_name linux)
else()
    message(FATAL_ERROR "Unsupported operating system: ${CMAKE_SYSTEM_NAME}")
endif()

# Downloads, verifies, and unpacks a binary dependency from AWS. If SCIN_CLOBBER_BINARY_DEPS is true, then remove it
# even if it exists; otherwise if SCIN_CHECK_BINARY_DEPS_UPDATES is true check and inform the user if a newer dep
# version is available; otherwise only act if the directory doesn't exist yet.
function(DownloadBinaryDep dep_name)
    set(dep_dir ${CMAKE_CURRENT_SOURCE_DIR}/${dep_name})
    if(EXISTS ${dep_dir})
        if(SCIN_CLOBBER_BINARY_DEPS)
            message(VERBOSE "Removing preexisting folder for ${dep_name} at ${dep_dir}")
            file(REMOVE_RECURSE ${dep_dir})
        elseif(NOT SCIN_CHECK_BINARY_DEPS_UPDATES)
            message(VERBOSE "Skipping check for ${dep_name} updates")
            message(VERBOSE "Skipping setup for ${dep_name}: directory ${dep_dir} exists")
            return()
        endif()
    endif()

    # We use this later to determine whether to note an update if a new version is available.
    if(EXISTS ${dep_dir})
        set(dep_dir_exists 1)
    else()
        set(dep_dir_exists 0)
    endif()

    file(MAKE_DIRECTORY ${dep_dir}) # may already exist, no error posted

    # Download xyz-latest.html and parse it to find the real ZIP URL
    set(dep_latest_file_name ${dep_name}-${dep_os_name}-latest.html)
    set(dep_latest_url http://scintillator-synth-coverage.s3-website-us-west-1.amazonaws.com/binaries/${dep_name}/${dep_latest_file_name})
    set(dep_latest_file ${dep_dir}/${dep_latest_file_name})
    message(VERBOSE "Download latest-redirect from ${dep_latest_url}")
    file(DOWNLOAD ${dep_latest_url} ${dep_latest_file})
    file(STRINGS ${dep_latest_file} dep_latest_html_contents)
    string(REGEX MATCH "http://.*\\.tgz" dep_zip_url ${dep_latest_html_contents})
    message(VERBOSE "URL from latest-redirect: ${dep_zip_url}")
    if(NOT dep_zip_url)
        message(FATAL_ERROR "No redirect URL found in file from ${dep_latest_url}, contents: ${dep_latest_html_contents}")
    endif()

    # Check for updates; if we are set to not clobber and the directory already existed, this is where we bail out
    get_filename_component(dep_zip_file_name ${dep_zip_url} NAME)
    set(dep_zip_file ${dep_dir}/${dep_zip_file_name})
    if(dep_dir_exists)
        if(NOT EXISTS ${dep_zip_file})
            message(STATUS "Update is available for ${dep_name}: ${dep_zip_file}")
            message(STATUS "Remove ${dep_dir} and rerun CMake configuration to install it")
        else()
            message(STATUS "No update available for ${dep_name}")
        endif()
        return()
    endif()

    message(STATUS "Downloading and unpacking ${dep_name} in ${dep_dir}")

    # Get expected hash for zip
    string(REGEX REPLACE "\\.tgz$" ".sha256" dep_hash_url "${dep_zip_url}")
    string(REGEX REPLACE "\\.tgz$" ".sha256" dep_hash_file "${dep_zip_file}")
    message(VERBOSE "Download ${dep_hash_url} to ${dep_hash_file}")
    file(DOWNLOAD ${dep_hash_url} ${dep_hash_file})
    file(READ ${dep_hash_file} dep_hash_file_contents LIMIT 64)
    string(STRIP ${dep_hash_file_contents} dep_expected_hash) # strip trailing newline added by file(READ)
    message(VERBOSE "Got hash: ${dep_expected_hash}")

    # Download and unpack zip
    message(VERBOSE "Download ${dep_zip_url} to ${dep_zip_file_name}")
    file(DOWNLOAD ${dep_zip_url} ${dep_zip_file} SHOW_PROGRESS EXPECTED_HASH SHA256=${dep_expected_hash})
    message(VERBOSE "Decompress ${dep_zip_file}")
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xz ${dep_zip_file}
        OUTPUT_VARIABLE tar_out ERROR_VARIABLE tar_err WORKING_DIRECTORY ${dep_dir})
    message(VERBOSE "tar stdout: ${tar_out}\ntar stderr: ${tar_err}")

    message(STATUS "Finished unpacking ${dep_name}")
endfunction()

# Provides a target `lib_name` with appropriate include and link properties.
function(CreateBinaryDepLibrary dep_name lib_name alt_lib_name)
    message(VERBOSE "Creating binary-dep library ${lib_name}")
    find_library(${lib_name}_library
        NAMES ${lib_name} ${alt_lib_name}
        PATHS ${dep_name}
        PATH_SUFFIXES lib bin
        NO_DEFAULT_PATH
    )
    # "interface" libraries only have an interface, no build steps
    add_library(${lib_name} INTERFACE IMPORTED GLOBAL)
    target_include_directories(${lib_name} INTERFACE ${dep_name}/include)
    target_link_libraries(${lib_name} INTERFACE ${${lib_name}_library})
endfunction()

#### Real commands

# Download dependencies
DownloadBinaryDep(vulkan-ext)
DownloadBinaryDep(ffmpeg-ext)
DownloadBinaryDep(swiftshader-ext)

# Force using the locally sourced libraries as opposed to system libs
CreateBinaryDepLibrary(vulkan-ext shaderc_combined "")
CreateBinaryDepLibrary(vulkan-ext vulkan vulkan-1)
CreateBinaryDepLibrary(ffmpeg-ext avcodec "")
CreateBinaryDepLibrary(ffmpeg-ext avformat "")
CreateBinaryDepLibrary(ffmpeg-ext avutil "")
CreateBinaryDepLibrary(ffmpeg-ext swscale "")

option(SCIN_CHECK_BINARY_DEPS_UPDATES "Check on every configure step whether a new version of each binary dependency is available" OFF)
option(SCIN_CLOBBER_BINARY_DEPS "Forcibly redownload and unpack binary dependencies on every configure step" OFF)
