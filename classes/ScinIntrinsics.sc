// Intrinsics are things that are built in to the graphics pipeline,
// or can be provided by the graphics pipeline if configured to do so,
// such as fragment or vertex coordinates, and sampler coordinates.
ScinIntrinsic : PureVGen {
	var <>intrinsic;

	*intrinsicNew { | rate, intrinsicName |
		^this.multiNew(rate).intrinsic_(intrinsicName);
	}

}

XPos : ScinIntrinsic {
	*fg {
		^super.intrinsicNew(\fragment, \x_pos);
	}

	*vx {
		^super.intrinsicNew(\vertex, \x_pos);
	}
}

YPos : ScinIntrinsic {
	*fg {
		^super.intrinsicNew(\fragment, \y_pos);
	}

	*vx {
		^super.intrinsicNew(\vertex, \y_pos);
	}
}

ZPos : ScinIntrinsic {
	*fg {
		^super.intrinsicNew(\fragment, \z_pos);
	}

	*vx {
		^super.intrinsicNew(\vertex, \z_pos);
	}
}

WPos : ScinIntrinsic {
	*fg {
		^super.intrinsicNew(\fragment, \w_pos);
	}

	*vx {
		^super.intrinsicNew(\vertex, \w_pos);
	}
}
