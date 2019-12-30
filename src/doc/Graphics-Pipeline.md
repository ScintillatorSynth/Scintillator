The Scintillator Graphics Pipeline {#GraphicsPipeline}
==================================

Shape (Quad only for now) -> Transform (orthographic only for now) -> Color (fragment shader only ScinthDef) ->
Composite

Shape is vertex and index buffer, with attachments to the vertex buffer as necessary.

Transform is the tesselation and vertex shader stages.

Color is the fragment shader stage, may also include compositing (when rendering directly to output buffer).


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

