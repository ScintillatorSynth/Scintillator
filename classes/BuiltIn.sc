VClamp : VGen {
	*fr { |v, min, max|
		^this.multiNew(\frame, v, min, max);
	}

	*sr { |v, min, max|
		^this.multiNew(\shape, v, min, max);
	}

	*pr { |v, min, max|
		^this.multiNew(\pixel, v, min, max);
	}

	inputDimensions {
		^[[1, 1, 1], [2, 2, 2], [3, 3, 3], [4, 4, 4]];
	}

	outputDimensions {
		^[[1], [2], [3], [4]];
	}
}

VCross : VGen {
	*fr { |u, v|
		^this.multiNew(\frame, u, v);
	}

	*sr { |u, v|
		^this.multiNew(\shape, u, v);
	}

	*pr { |u, v|
		^this.multiNew(\pixel, u, v);
	}

	inputDimensions {
		^[[3, 3]];
	}

	outputDimensions {
		^[[3]];
	}
}

VDot : VGen {
	*fr { |u, v|
		^this.multiNew(\frame, u, v);
	}

	*sr { |u, v|
		^this.multiNew(\shape, u, v);
	}

	*pr { |u, v|
		^this.multiNew(\pixel, u, v);
	}

	inputDimensions {
		^[[1, 1], [2, 2], [3, 3], [4, 4]];
	}

	outputDimensions {
		^[[1], [1], [1], [1]];
	}
}

VDistance : VGen {
	*fr { |u, v|
		^this.multiNew(\frame, u, v);
	}

	*sr { |u, v|
		^this.multiNew(\shape, u, v);
	}

	*pr { |u, v|
		^this.multiNew(\pixel, u, v);
	}

	inputDimensions {
		^[[1, 1], [2, 2], [3, 3], [4, 4]];
	}

	outputDimensions {
		^[[1], [1], [1], [1]];
	}
}

VFragCoord : VGen {
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

VLength : VGen {
	*fr { |v|
		^this.multiNew(\frame, v);
	}

	*sr { |v|
		^this.multiNew(\shape, v);
	}

	*pr { |v|
		^this.multiNew(\pixel, v);
	}

	inputDimensions {
		^[[1], [2], [3], [4]];
	}

	outputDimensions {
		^[[1], [1], [1], [1]];
	}
}

VStep : VGen {
	*fr { |step, v|
		^this.multiNew(\frame, step, v);
	}

	*sr { |step, v|
		^this.multiNew(\shape, step, v);
	}

	*pr { |step, v|
		^this.multiNew(\pixel, step, v);
	}

	inputDimensions {
		^[[1, 1], [2, 2], [3, 3], [4, 4]];
	}

	outputDimensions {
		^[[1], [2], [3], [4]];
	}
}

VVecMix : VGen {
	*fr { |u, v, a|
		^this.multiNew(\frame, u, v, a);
	}

	*sr { |u, v, a|
		^this.multiNew(\shape, u, v, a);
	}

	*pr { |u, v, a|
		^this.multiNew(\pixel, u, v, a);
	}

	inputDimensions {
		^[[1, 1, 1], [2, 2, 2], [3, 3, 3], [4, 4, 4], [2, 2, 1], [3, 3, 1], [4, 4, 1]];
	}

	outputDimensions {
		^[[1], [2], [3], [4], [2], [3], [4]];
	}
}

VNorm : VGen {
	*fr { |v|
		^this.multiNew(\frame, v);
	}

	*sr { |v|
		^this.multiNew(\shape, v);
	}

	*pr { |v|
		^this.multiNew(\pixel, v);
	}

	inputDimensions {
		^[[1], [2], [3], [4]];
	}

	outputDimensions {
		^[[1], [2], [3], [4]];
	}
}