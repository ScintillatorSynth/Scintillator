project(ScintillatorSynth)

cmake_minimum_required(VERSION 2.6)

option(SCIN_VALIDATE_VULKAN
    "Use LunarG Vulkan Validation Layers to check Vulkan calls" ON)

add_subdirectory(third_party)
add_subdirectory(src)

