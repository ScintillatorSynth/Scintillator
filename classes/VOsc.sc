VSinOsc : VGen {
	*fr { |freq = 1.0, phase = 0.0, mul = 0.5, add = 0.5|
		^this.multiNew(\frame, freq, phase, mul, add);
	}

	*sr { |freq = 1.0, phase = 0.0, mul = 0.5, add = 0.5|
		^this.multiNew(\shape, freq, phase, mul, add);
	}

	*pr { |freq = 1.0, phase = 0.0, mul = 0.5, add = 0.5|
		^this.multiNew(\pixel, freq, phase, mul, add);
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
		^this.multiNew(\frame, freq, phase);
	}

	*sr { |freq = 1.0, phase = 0.0|
		^this.multiNew(\shape, freq, phase);
	}

	*pr { |freq = 1.0, phase = 0.0|
		^this.multiNew(\pixel, freq, phase);
	}

	inputDimensions {
		^[[1, 1], [2, 2], [3, 3], [4, 4]];
	}

	outputDimensions {
		^[[1], [2], [3], [4]];
	}
}
