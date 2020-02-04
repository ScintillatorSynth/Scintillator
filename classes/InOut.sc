RGBOut : VGen {
	*fg { |r, g, b|
		^this.multiNew(\fragment, r, g, b);
	}

	inputDimensions {
		^[[1, 1, 1]];
	}

	outputDimensions {
		^[[4]];
	}
}

BWOut : VGen {
	*fg { |v|
		^this.multiNew(\fragment, v);
	}

	inputDimensions {
		^[[1]];
	}

	outputDimensions {
		^[[4]];
	}
}

RGBAOut : VGen {
	*fg { |r, g, b, a|
		^this.multiNew(\fragment, r, g, b, a);
	}

	inputDimensions {
		^[[1, 1, 1, 1]];
	}

	outputDimensions {
		^[[4]];
	}
}

VControl : MultiOutVGen {
	var <>names;
	var <>defaultValues;

	// TODO: it is not accurate to call these fragment rate.
	*fg { |values|
		^this.multiNewList([\fragment] ++ values.asArray);
	}

	init { |...values|
		^this.initOutputs(values.size, rate);
	}

	isControlVGen { ^true }

	inputDimensions {
		^[[]];
	}

	outputDimensions {
		^[[1].stutter(names.size)];
	}
}