VSinOsc : VGen {
	// Fragment rate - computed once per "pixel" via a fragment shader.
	*fg { | freq = 1.0, phase = 0.0, mul = 1.0, add = 0.0 |
		^this.multiNew(\fragment, freq, phase).madd(mul, add);
	}

	// Vertex rate - computed once per vertex via a vertex shader, then
	// interpolated by the graphics hardware to per-fragment values.
	*vx {
		^thisMethod.netYetImplemented;
	}
}