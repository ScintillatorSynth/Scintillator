Scinth Generation {#ScinthGeneration}
=================

# From Input YAML to AbstractScinthDef

A running Scinth consists of a collection of Vulkan data structures and commands that are executed every frame in order
to produce the visual result. In order to support rendering Scinths with maximum efficiency we prepare as much of the
Vulkan state ahead of time as possible during the ScinthDef preparation process.

This process is broken into two phases. The first phase does not depend on Vulkan code and consists largely of shader
program generation and gathering of requirements in order to create the Vulkan-specific ScinthDef. The output of the
first phase is an AbstractScinthDef object that contains all of the information necessary to generate a ScinthDef.

The two-part process allows the code in the core/ directory, where AbstractScinthDef and its related objects reside, to
not depend on Vulkan and to therefore be readily unit-testable. Once the Vulkan code is introduced the testing strategy
shifts to integration testing with Vulkan validation layers and Swiftshader, ensuring that the scinsynth usage of the
Vulkan API is correct with the spec and working as intended.

# Phase 1: From Input YAML to AbstractScinthDef

The AbstractScinthDef is intended to be a complete specification of all graphics configuration and commands needed to
render the specified ScinthDef. The list of specifications includes:

* Source strings for both vertex and fragment shaders.
* A manifest for a vertex buffer, meaning what additional variables are intended to be included in the vertex buffer.
* A manifest for a uniform buffer, meaning names, dimensions, offsets in order of each element in the uniform, as well
  as an enumeration detailing if the buffer needs to be supplied to the vertex, fragment, or both shaders.
* A list of all Intrinsics the ScinthDef depends on.
* The named arguments and their dimensions.

Arguments are provided via Vulkan Push Constants. Every Scinth creates its own Command Buffer and can recreate it as
needed on argument change. If an argument is to change frequently users are advised to attach to Busses, which can be
provided as uniforms and therefore updated every frame as part of the uniform update block (via Intrinsics @bus).

# Temporary Variables -

Because shaders can have inputs and outputs of varying dimension in order to declare a variable of the same dimension of
a given input or output there needs to be some intrinsic like @typeof() that takes an argument. This will have to wait
until another iteration of the ScinthDef generator.

