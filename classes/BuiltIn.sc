Clamp : VGen {
	*fg { |x, min, max|
		^this.multiNew(\fragment, x, min, max);
	}

	inputDimensions {
		^[[1, 1, 1], [2, 2, 2], [3, 3, 3], [4, 4, 4]];
	}

	outputDimensions {
		^[[1], [2], [3], [4]];
	}
}

FragCoord : VGen {
	*fg {
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
	*fg { |vec|
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
	*fg { |step, x|
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
	*fg { |x, y, a|
		^this.multiNew(\fragment, x, y, a);
	}

	inputDimensions {
		^[[1, 1, 1], [2, 2, 2], [3, 3, 3], [4, 4, 4], [2, 2, 1], [3, 3, 1], [4, 4, 1]];
	}

	outputDimensions {
		^[[1], [2], [3], [4], [2], [3], [4]];
	}
}
