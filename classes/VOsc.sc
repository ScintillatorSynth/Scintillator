VSinOsc : VGen {
	*fr { |freq = 1.0, phase = 0.0, mul = 0.5, add = 0.5|
		^this.multiNew(\fragment, freq, phase, mul, add);
	}

	inputDimensions {
		^[[1, 1, 1, 1], [2, 2, 2, 2], [3, 3, 3, 3], [4, 4, 4, 4]];
	}

	outputDimensions {
		^[[1], [2], [3], [4]];
	}
}

VSaw : VGen {
	*fr { |freq = 1.0, phase = 0.0|
		^this.multiNew(\fragment, freq, phase);
	}

	inputDimensions {
		^[[1, 1], [2, 2], [3, 3], [4, 4]];
	}

	outputDimensions {
		^[[1], [2], [3], [4]];
	}
}
