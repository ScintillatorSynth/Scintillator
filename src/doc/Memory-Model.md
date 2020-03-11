scinsynth Vulkan Memory Model
=============================

Best practice in Vulkan is to upload images to staging buffers, then transfer to VkImage with tiling set to OPTIMAL.

Some devices maintain an independent transfer queue, which is great because we can run the transfers and layout
transitions on an independent thread. However, the Vulkan spec offers no guarantee that there will always be a
dedicated queue.

On devices with a dedicated queue, the StageManager object can complete transfers entirely on its own dedicated thread.
On devices with a shared queue, it can construct the CommandBuffers on that thread, this time as secondary command
buffers, for submission to the Window/Offscreen Graphics queue.

* Decision: command buffers are always primary, batching all available transfer requests into a single command buffer.
* Decision: can build the independent transfer queue stuff later
(https://github.com/ScintillatorSynth/Scintillator/issues/61)


