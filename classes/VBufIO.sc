IBufRd : VGen {
	*fg { |image, pos, sampler = 0|
		^this.multiNew(\fragment, image, pos, sampler);
	}

	inputDimensions {
		^[[1, 2, 1]];
	}

	outputDimensions {
		^[[4]];
	}

	inputValueType { |index|
		var type = switch (index,
			0, { \imageBuffer },
			1, { \float },
			2, { \sampler });
		^type;
	}
}