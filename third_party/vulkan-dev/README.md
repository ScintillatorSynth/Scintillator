Vulkan Development Utilities
============================

This directory contains build instructions for three different components:

 * A standalone Vulkan SDK consisting of the Vulkan Headers and Loader.
 * The Vulkan Validation Layers, which attach to a running Vulkan process and can validate every Vulkan function call.
 * Swiftshader, a CPU-only Vulkan software renderer, useful for regression testing because it can be expected to produce
   consistent images in a machine-independent fashion and works on hardware that doesn't have any graphics capability
   installed or configured (e.g. Travis CI build machines).

Building against a locally-installed Vulkan SDK is the default option, so the standalone Vulkan SDK is only built when
the flag ```SCIN_USE_OWN_VULKAN``` is on. The Vulkan Validation Layers will require a Vulkan SDK to build, swiftshader
does not.

TODO: add some notes about building here, essentially it's just make a build subdirectory, cd in to it, cmake ..,
make install for the vulkan SDK, and make local-vulkan-utils to locally install swiftshader and the validation layers.
