project(ScintillatorSynth)

cmake_minimum_required(VERSION 2.6)

add_subdirectory(third_party)
add_subdirectory(src)

#### Vulkan
find_package(Vulkan)
if (NOT VULKAN_FOUND)
  message(FATAL_ERROR "Vulkan not found")
endif()

