VRGBOut : VGen {
	*pr { |red, green, blue|
		^this.multiNew(\pixel, red, green, blue);
	}

	inputDimensions {
		^[[1, 1, 1]];
	}

	outputDimensions {
		^[[4]];
	}
}

VBWOut : VGen {
	*pr { |value|
		^this.multiNew(\pixel, value);
	}

	inputDimensions {
		^[[1]];
	}

	outputDimensions {
		^[[4]];
	}
}

VRGBAOut : VGen {
	*pr { |red, green, blue, alpha|
		^this.multiNew(\pixel, red, green, blue, alpha);
	}

	inputDimensions {
		^[[1, 1, 1, 1]];
	}

	outputDimensions {
		^[[4]];
	}
}

VControl : MultiOutVGen {
	var <>values;

	*new { |values|
		// Rate is ignored in the ScinthDef construction.
		^this.multiNewList([\param] ++ values.asArray);
	}

	init { |...argValues|
		values = argValues;
		^this.initOutputs(values.size, rate);
	}

	addToScinth {
		scinthDef = buildScinthDef;
		// Although we are a VGen we should not be a part of the VGen graph, so we
		// don't add ourselves to it here.
	}

	isControlVGen { ^true }

	inputDimensions {
		^[[]];
	}

	outputDimensions {
		^[[1].stutter(outputs.size)];
	}
}
