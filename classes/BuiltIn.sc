Clamp : VGen {
	*fr { |x, min, max|
		^this.multiNew(\frame, x, min, max);
	}

	*sr { |x, min, max|
		^this.multiNew(\shape, x, min, max);
	}

	*pr { |x, min, max|
		^this.multiNew(\pixel, x, min, max);
	}

	inputDimensions {
		^[[1, 1, 1], [2, 2, 2], [3, 3, 3], [4, 4, 4]];
	}

	outputDimensions {
		^[[1], [2], [3], [4]];
	}
}


Cross : VGen {
	*fr { |x, y|
		^this.multiNew(\frame, x, y);
	}

	*sr { |x, y|
		^this.multiNew(\shape, x, y);
	}

	*pr { |x, y|
		^this.multiNew(\pixel, x, y);
	}

	inputDimensions {
		^[[3, 3]];
	}

	outputDimensions {
		^[[3]];
	}
}

Dot : VGen {
	*fr { |x, y|
		^this.multiNew(\frame, x, y);
	}

	*sr { |x, y|
		^this.multiNew(\shape, x, y);
	}

	*pr { |x, y|
		^this.multiNew(\pixel, x, y);
	}

	inputDimensions {
		^[[1, 1], [2, 2], [3, 3], [4, 4]];
	}

	outputDimensions {
		^[[1], [1], [1], [1]];
	}
}

Distance : VGen {
	*fr { |x, y|
		^this.multiNew(\frame, x, y);
	}

	*sr { |x, y|
		^this.multiNew(\shape, x, y);
	}

	*pr { |x, y|
		^this.multiNew(\pixel, x, y);
	}

	inputDimensions {
		^[[1, 1], [2, 2], [3, 3], [4, 4]];
	}

	outputDimensions {
		^[[1], [1], [1], [1]];
	}
}

FragCoord : VGen {
	*fr {
		^this.multiNew(\frame);
	}

	*sr {
		^this.multiNew(\shape);
	}

	*pr {
		^this.multiNew(\pixel);
	}

	inputDimensions {
		^[[]];
	}

	outputDimensions {
		^[[2]];
	}
}

Length : VGen {
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

Step : VGen {
	*fr { |step, x|
		^this.multiNew(\frame, step, x);
	}

	*sr { |step, x|
		^this.multiNew(\shape, step, x);
	}

	*pr { |step, x|
		^this.multiNew(\pixel, step, x);
	}

	inputDimensions {
		^[[1, 1], [2, 2], [3, 3], [4, 4]];
	}

	outputDimensions {
		^[[1], [2], [3], [4]];
	}
}

VecMix : VGen {
	*fr { |x, y, a|
		^this.multiNew(\frame, x, y, a);
	}

	*sr { |x, y, a|
		^this.multiNew(\shape, x, y, a);
	}

	*pr { |x, y, a|
		^this.multiNew(\pixel, x, y, a);
	}

	inputDimensions {
		^[[1, 1, 1], [2, 2, 2], [3, 3, 3], [4, 4, 4], [2, 2, 1], [3, 3, 1], [4, 4, 1]];
	}

	outputDimensions {
		^[[1], [2], [3], [4], [2], [3], [4]];
	}
}

VNorm : VGen {
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
		^[[1], [2], [3], [4]];
	}

	outputDimensions {
		^[[1], [2], [3], [4]];
	}
}