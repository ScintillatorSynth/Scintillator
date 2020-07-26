VNormPos : VGen {
	*sr {
		^this.multiNew(\shape);
	}

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

VTexPos : VGen {
	*sr {
		^this.multiNew(\shape);
	}

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
