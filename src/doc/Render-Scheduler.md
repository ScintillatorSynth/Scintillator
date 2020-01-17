Render Scheduler {#RenderScheduler}
================

Scintillator may or may not create a window, as it is capable of rendering to an offscreen context, with the likely
intention of encoding some or all of the generated frames to still images or video. Window creation is specified with
the --window boolean flag at startup, which defaults to true. To determine output framerate, Scintillator can be started
in one of three modes:

 * Free-running - only relevant when a window is created, instructs Scintillator to track the update framerate of the
   display. Pass a negative number to the frame rate startup flag to activate this mode.
 * Shutter - Scintillator will only advance the frame when receiving an OSC command requesting a render. If there is a
   window open, the window will update the contents when "damaged", or a repaint is requested by the operating system,
   and will attempt to present the updated contents with minimal latency.
 * Fixed - Scintillator will render with a fixed framerate, which can be any integer number of frames per second. This
   number can be significantly smaller or larger than the display frame rate if a window is open, for example the
   requested frame rate could be as low as 1 frame per second or as high as 240 fps. For framerates lower than the
   display frame rate the window will attempt to present updated frames with minimal latency, and for higher frame rates
   Window will not present every frame but will rather present the most recently rendered frame, which may cause higher
   frame rates to lag behind real time significantly.

Free-Running Mode
-----------------

The thinking is that free-running mode is intended for live performance, and Scintillator should make every effort to
render with lowest latency, which comes at the risk of dropped frames. Dropped frames in other graphics contexts are
typically prevented by increasing buffering, but this unfortunately also increases the latency between rendering and
presentation, as the frame is rendered, then enters a queue for presentation, and is presented in order some time later.

In this mode we render directly to the swapchain images and measure times between getting through vulkan fences
indicating the image is ready for rendering again. This means that the image has finshed presenting, and therefore in
its lifecycle that is the earliest we could begin rendering to it. Once we clear the fence we measure the time. If its
significantly greater than the average we report that as a dropped frame (will need to rate-limit this message or build
a counter of number of times we were late and then report that periodically). We use the high precision clock and report
accurate timestamps on each frame render.

The swapchain *has* to be configured in FIFO mode for this to work, because in MAILBOX mode the present queue will happy
accept infinite new frames, just replacing the old one in the queue with the newer one.

Numbers lower than -1 are intended to represent a multiple of the frame time, but that work is TBD. So if the display is
refreshing at 60Hz a frame rate argument of -2 would request window-locked rendering at 30 Hz. This would probably be
implemented with an offscreen renderer and a blit?

Fixed and Shutter Mode
----------------------

The Offscreen class maintains a root Compositor and schedules render to the framebuffers. Because frame latency is much
less of a concern in non real time mode, the Offscreen class pipelines rendering. The product of offscreen rendering is
one or more copies of the render can be made each frame. Media encoding requires that the framebuffer either be
host-accessible itself, or blitted to another framebuffer that is host-accessible. The contents are then transfered to
an AVFrame and sent off to one or more encoders.

One tricky bit is that we only want to pipeline if we are animating. If we're single shot it doesn't make sense to
pipeline, because nothing will come out of the pipe until several more calls to render() are made. So there's a bool
we keep, something like ```bool flush```, that tells the render loop if it should wait on the fence after submit or not.

If we're animating, we wait on the fence right before re-rendering. We can then map the copy, readback the data for any
encodes, and think about updating state about swapchain source imagery. Maybe we keep a vector per-image of pending
copies? That way if we flush on a frame we understand that we don't necessarily have to redo the copy.

Per-pipelined image we keep a host-accessible image (which can be the framebuffer directly for CPU rendering, likely),
but only blit to it when an encode is requested.

If there's a swapchain in the mix, we try and create a pair of images (or 3?), GPU accessible only, that are in a format
that makes blit from them to the swapchain framebuffer possible. When the Window requests a swapchain transfer source
image, the Offscreen needs to consider that image "locked" until swapchain requests another image and is provided with
a different one. So it seems these swapchain source images have three states - "locked," meaning that this is the
current swapchain image we are providing, "waiting for blit," in which case a GPU command has been sent to blit to it,
but we haven't waited on the fence for that gets cleared when the render is known finished, and "ready," which is the
state where we know it has fresh data but hasn't been provided to the swapchain yet (because swapchain hasn't asked).

So one image will be locked, and one will be either waiting for blit or ready. Probably only two images needed, and only
need to be created when asked for by Swapchain or Window, who can also provide a good format suggestion for blit source.




