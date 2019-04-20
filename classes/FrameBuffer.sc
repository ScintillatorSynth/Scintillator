// Although it has Buffer in the name a FrameBuffer in Scintillator
// has a lot more to do with a Bus in SuperCollider audio. It specifies
// a kind of VideoBuffer that can serve as a render target. The server
// always creates a series of FrameBuffers by default for buffered
// rendering to the output. Additional FrameBuffers can be created
// for render targets.
FrameBuffer : ImageBuffer {
}