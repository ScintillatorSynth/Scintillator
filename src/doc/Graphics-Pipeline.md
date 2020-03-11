The Scintillator Graphics Pipeline {#GraphicsPipeline}
==================================

Shape (Quad only for now) -> Transform (orthographic only for now) -> Color (fragment shader only ScinthDef) ->
Composite

Shape is vertex and index buffer, with attachments to the vertex buffer as necessary.

Transform is the tesselation and vertex shader stages.

Color is the fragment shader stage, may also include compositing (when rendering directly to output buffer).

Multi-Stage Rendering
---------------------

Current VGens take as input a single floating-point value, which can be a constant, parameter, or output from another
VGen also providing a single floating-point value. Intrinsics like NormPos and TexPos allow for VGens to vary their
output as a function of position in the image as well as the values of other inputs in the graph, but right now the
computation of a color of a given pixel is only a function of its position and time.

With the introduction of Image rendering this obviates the need for supporting VGens with color output that is a
function of *other fragment colors*. Generally we could call these *sampling VGens* or *visual filters* because they
need to *sample* other image contexts. This is different than alpha blending, which can be implemented in render
subpasses, because the fragment only needs to know the current color of itself.

A great example of a sampling filter that obviously needs access to other fragments is a simple blur filter, which
typically computes the gaussian weighted average of the pixels in the area around fragment. One might want to blur
either a still image or a dynamic one.

So something like (with very tentative VGen names):

```
RGBOut.fg(Blur4.fg(<some complicated VGen graph>));
```

The system needs to understand that Blur4 is a sampler and therefore needs things that occur before Blur4 to happen in
their own render pass. That could be considered an implicit Framebuffer creation, and Framebuffers here connect back a
bit to Busses scsynth.

Generally there's Image data from multiple sources. It can be computed dynamically by other external synths. It can be
computed by a part of the same Scinth that is dependent on it, like in the above Blur example. Or it can be decoded from
video or still image media. All of these require Sampling behavior and special handling by the Compositor.

Blends (no dependencies on other fragment colors):
    happen between Scinths, different blend algorithms could be configured. Must happen in render subpasses.

Sampling (must sample other fragment colors):
    sampled image must be configured as a dependency of the shader/scinth. If the sampled image is being created by
    a sub-part of the Scinth that part will have to be rendered in its own render pass.


Proposal is to add an additional call to Compositor and Scinths that returns a vector of command buffers, and may also
do some CPU-related work, called updateDependentImages(). This will process any SubScinths, meaning Scinths that are
sampled and so need to be rendered to separate framebuffer image. Also any staging operations could happen in own
command buffers. Remember - command buffers happen in submission order, but individual commands with buffer may be
re-ordered/paralellized.

Frame render happens in severl distinct phases, three of which the Compositor API will support:

0) host-side computation - thinking video decode and frame sequencing. Also envelopes, which can determine a Scinth is
done and should be removed, or is opaque and therefore should be removed, or invariant and possibly could be cached,
etc.

a) Staging - on discrete GPUs we issue a single command buffer which can copy host-accessible buffer data into faster
device-accessible buffers. This is not a universal win for performance and is very architecture-dependent but in some
cases it is a clear win and the Compositor would issue the commands to do that in this step. Two sub-steps are host
staging and device staging, where images/video can be uploaded from host and then copied.

Another consideration for staging are any buffers that are relying on output from previous renders. Because of the
pipelined nature of the rendering we need to manually blit/copy these buffers from the previous frame to the next frame.
It is not safe to assume that the prior contents of any buffer are valid or represent the state of the previous frame
render.

b) Dependent Image rendering - Offscreen rendering to a framebuffer is an implementation detail of several different
usage scenarios. They can be created implicitly by describing a VGen graph that includes a sampler that is sampling
output of a subgraph, like the Blur4 example described above. They could also be created explicitly, seeming the
logical means to support Scintillator equivalents to LocalIn/Out, and Busses.

One can construct a dependency tree of VGens and output framebuffers inside of each Scinth. Then the Scinths are
rendered in client-specified order. If the max depth of the forest of individual tree Scinths is N framebuffers,
including the final output framebuffer, then the Dependent Image render is everything but the final output framebuffers,
consisting of N-1 command buffers.

(picture here would be helpful)

Because command buffers are executed in order but individual commands inside of a buffer may be executed in parallel, we
design the system to do a depth-order traversal of the render tree and collect each layer of the tree into a common
command buffer. This allows the individual images on a given depth of the tree to render in parallel (if the GPU sees
fit), but maintains the dependency between depths by specifying the next depth render in a subsequent command buffer.

c) Base Scinth Rendering - with buffers all situated correctly thanks to the staging steps, and populated with any
dependent renders thanks to dependent image rendering, the primary output from Scinths can now be combined by rendering
directly into the output Framebuffer.

Here things are grouped into one command buffer with one render pass, and a render subpass per Scinth, to allow alpha
blending to occur between the final Scinth outputs. Note that alpha blending is just implied, VGens just supply values
<1 for alpha on individual fragments and the Compositor will provide for the blending.

Culling
-------

Temporal culling - any Scinth that doesn't have @time as an intrinsic can be rendered to a Framebuffer and then only
updated when parameters change.

Opacity culling - any Scinth with output that will be completely covered by the opaque output of another scinth can be
completely covered. Individual VGens should specify if their output is opaque, and the final output VGen of a Scinth
can be evaluated. More can be done here to infer opacity, but even just turning off rendering for Scinths that are
explicitly opaque can be helpful. Once vertex transformations are enabled and stuff can move around this will have to
consider that as well using bounding boxes.

Ok.. So What does all that mean for Image rendering?
----------------------------------------------------

Current effort is around textures and images. So first is to think through what a VGen spec looks like for a Sampler
both on the AbstractVGen side as well as the ScinthDef file format. Let's list here the options for sampler creation:

 * magFilter, minFilter (p2)
 * addressing modes for u and v (p0)
 * anisotropic filtering (p1)
 * border color enum (p0)
 * unnormalized/normalized coordinates (p0)


Descriptor Pools, Sets, SetLayouts
----------------------------------

Each Scinth keeps its own Descriptor Pool, because ScinthDef does not know at creation time how many Scinths will be
allocated, and thus how many descriptor sets to create.

The Set Layout can live with the ScinthDef, however. Action is to deprecate the UniformLayout class and move the logic
directly into ScinthDef, or possibly to update it to support the number of different images and samplers coming in.

DECISION: deprecate UniformLayout and Uniform, fold their work into ScinthDef and Scinth prespectively.

Dependencies of Vulkan Objects
------------------------------
Everything depends on Device, so all of these structures are device-specific.

Command Buffer: (1 for each image) - clearly set per Scinth instance
    Command Pool
    Render Pass
    Framebuffer
    Extent
    Pipeline
    Vertex Buffer, Index Buffer, Uniforms
    Clear Color

Pipeline: - probably 1 per ScinthDef and Canvas instance
    Vertex Binding, Vertex Topology
    Extent (for viewport setup) (provided by renderable?)
    Uniform setup
    Shaders
    Render Pass (provided by renderable?)

** Seems like the below 3 fit together pretty nicely into something like a RenderPass or Renderable: **
(or maybe a compositor!) (Canvas?)

Render Pass:
    Surface Format

Framebuffers: (currently made by SwapChain, 1 per ImageView)
    Image View
    Width and Height
    Render Pass

Image View: (currently made by SwapChain, 1 per Image)
    Image
    Surface Format

** Owned by either Window (for realtime, then has a SwapChain), or some kind of Offscreen render target **

Image: (currently extracted from SwapChain via vkGetSwapchainImagesKHR)
    SwapChain - which needs a surface, extent (width, height), other internals.

To draw to a window:
    need some fences to determine state of images.
    need a command buffer. (Scinth)
    need to update uniforms. (Scinth)
    submit to the queue. (device)


