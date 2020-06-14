Vec2 : VGen {
	*fr { |x = 0.0, y = 0.0|
		^this.multiNew(\frame, x, y);
	}

	*sr { |x = 0.0, y = 0.0|
		^this.multiNew(\shape, x, y);
	}

	*pr { |x = 0.0, y = 0.0|
		^this.multiNew(\pixel, x, y);
	}

	inputDimensions {
		^[[1, 1]];
	}

	outputDimensions {
		^[[2]];
	}
}

Vec3 : VGen {
	*fr { |x = 0.0, y = 0.0, z = 0.0|
		^this.multiNew(\frame, x, y, z);
	}

	*sr { |x = 0.0, y = 0.0, z = 0.0|
		^this.multiNew(\shape, x, y, z);
	}

	*pr { |x = 0.0, y = 0.0, z = 0.0|
		^this.multiNew(\pixel, x, y, z);
	}

	inputDimensions {
		^[[1, 1, 1]];
	}

	outputDimensions {
		^[[3]];
	}
}

Vec4 : VGen {
	*fr { |x = 0.0, y = 0.0, z = 0.0, w = 0.0|
		^this.multiNew(\frame, x, y, z, w);
	}

	*sr { |x = 0.0, y = 0.0, z = 0.0, w = 0.0|
		^this.multiNew(\shape, x, y, z, w);
	}

	*pr { |x = 0.0, y = 0.0, z = 0.0, w = 0.0|
		^this.multiNew(\pixel, x, y, z, w);
	}

	inputDimensions {
		^[[1, 1, 1, 1]];
	}

	outputDimensions {
		^[[4]];
	}
}

VX : VGen {
	*fr { |vec|
		^this.multiNew(\frame, vec);
	}

	*sr { |vec|
		^this.multiNew(\shape, vec);
	}

	*pr { |vec|
		^this.multiNew(\pixel, vec);
	}

	inputDimensions {
		^[[1], [2], [3], [4]];
	}

	outputDimensions {
		^[[1], [1], [1], [1]];
	}
}

VY : VGen {
	*fr { |vec|
		^this.multiNew(\frame, vec);
	}

	*sr { |vec|
		^this.multiNew(\shape, vec);
	}

	*pr { |vec|
		^this.multiNew(\pixel, vec);
	}

	inputDimensions {
		^[[2], [3], [4]];
	}

	outputDimensions {
		^[[1], [1], [1]];
	}
}

VZ : VGen {
	*fr { |vec|
		^this.multiNew(\frame, vec);
	}

	*sr { |vec|
		^this.multiNew(\shape, vec);
	}

	*pr { |vec|
		^this.multiNew(\pixel, vec);
	}

	inputDimensions {
		^[[3], [4]];
	}

	outputDimensions {
		^[[1], [1]];
	}
}


VW : VGen {
	*fr { |vec|
		^this.multiNew(\frame, vec);
	}

	*sr { |vec|
		^this.multiNew(\shape, vec);
	}

	*pr { |vec|
		^this.multiNew(\pixel, vec);
	}

	inputDimensions {
		^[[4]];
	}

	outputDimensions {
		^[[1]];
	}
}

Splat2 : VGen {
	*fr { |x|
		^this.multiNew(\frame, x);
	}

	*sr { |x|
		^this.multiNew(\shape, x);
	}

	*pr { |x|
		^this.multiNew(\pixel, x);
	}

	inputDimensions {
		^[[1]];
	}

	outputDimensions {
		^[[2]];
	}
}

Splat3 : VGen {
	*fr { |x|
		^this.multiNew(\frame, x);
	}

	*sr { |x|
		^this.multiNew(\shape, x);
	}

	*pr { |x|
		^this.multiNew(\pixel, x);
	}

	inputDimensions {
		^[[1]];
	}

	outputDimensions {
		^[[3]];
	}
}

Splat4 : VGen {
	*fr { |x|
		^this.multiNew(\frame, x);
	}

	*sr { |x|
		^this.multiNew(\shape, x);
	}

	*pr { |x|
		^this.multiNew(\pixel, x);
	}

	inputDimensions {
		^[[1]];
	}

	outputDimensions {
		^[[4]];
	}
}
