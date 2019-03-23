set(scintillator_synth_files
  scinsynth.cc
)

add_executable(scinsynth ${scintillator_synth_files})
target_link_libraries(scinsynth
  glfw
  ${Vulkan_LIBRARY}
)

