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