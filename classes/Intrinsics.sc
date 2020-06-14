NormPos : VGen {
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

TexPos : VGen {
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
