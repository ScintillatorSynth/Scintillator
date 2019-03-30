
#### Vulkan
find_package(Vulkan)
if (NOT VULKAN_FOUND)
    message(FATAL_ERROR "Vulkan not found")
endif()

set(scintillator_synth_files
    scin_include_vulkan.h
    scinsynth.cc
    vulkan_device.cc
    vulkan_device.h
    vulkan_instance.cc
    vulkan_instance.h
    vulkan_swap_chain.cc
    vulkan_swap_chain.h
    vulkan_window.cc
    vulkan_window.h
)

add_executable(scinsynth ${scintillator_synth_files})

target_link_libraries(scinsynth
    glfw
    shaderc_combined
    ${VULKAN_LIBRARY}
)

if (SCIN_VALIDATE_VULKAN)
    target_compile_definitions(scinsynth PRIVATE SCIN_VALIDATE_VULKAN)
endif()

