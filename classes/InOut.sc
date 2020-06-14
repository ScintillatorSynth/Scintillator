RGBOut : VGen {
	*pr { |r, g, b|
		^this.multiNew(\pixel, r, g, b);
	}

	inputDimensions {
		^[[1, 1, 1]];
	}

	outputDimensions {
		^[[4]];
	}
}

BWOut : VGen {
	*pr { |v|
		^this.multiNew(\pixel, v);
	}

	inputDimensions {
		^[[1]];
	}

	outputDimensions {
		^[[4]];
	}
}

RGBAOut : VGen {
	*pr { |r, g, b, a|
		^this.multiNew(\pixel, r, g, b, a);
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
