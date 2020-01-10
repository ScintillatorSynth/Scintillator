Render Scheduler {#RenderScheduler}
================

Scintillator may or may not create a window, as it is capable of rendering to an offscreen context, with the likely
intention of encoding some or all of the generated frames to still images or video. Window creation is specified with
the --window boolean flag at startup, which defaults to true. To determine output framerate, Scintillator can be started
in one of three modes:

 * Free-running - only relevant when a window is created, instructs Scintillator to track the update framerate of the
   display. Pass a negative number to the --framerate startup flag to activate this mode.
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



