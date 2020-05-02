Clamp : VGen {
	*fr { |x, min, max|
		^this.multiNew(\fragment, x, min, max);
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
		^this.multiNew(\fragment, x, y);
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
		^this.multiNew(\fragment, x, y);
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
		^this.multiNew(\fragment, x, y);
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
		^this.multiNew(\fragment);
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
		^this.multiNew(\fragment, vec);
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
		^this.multiNew(\fragment, step, x);
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
		^this.multiNew(\fragment, x, y, a);
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
		^this.multiNew(\fragment, x);
	}

	inputDimensions {
		^[[1], [2], [3], [4]];
	}

	outputDimensions {
		^[[1], [2], [3], [4]];
	}
}