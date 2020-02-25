// Base class for any VGen that samples pixel values outside of its own local value.
Sampler : VGen {
	// one of \repeat, \mirroredRepeat, \clampToEdge, or \clampToBorder
	var <>addressMode = \clampToBorder;
	// one of \transparentBlack, \black, \white, only used if addressMode is set to \clampToBorder.
	var <>clampBorderColor = \transparentBlack;

	var <>image;
	var <>imageArgType;

	*fg { |image, pos|
		var sampler = this.multiNew(\fragment, pos);
		case
		{ image.isNumber } {
			sampler.image = image;
			sampler.imageArgType = \constant;
		}
		{ image.isControlVGen } {
			sampler.image = image.outputIndex;
			sampler.imageArgType = \parameter;
		}
		{ Error.new("Unsupported argument type to Sampler image argument. Supported types are constants and parameters.").throw; };
		^sampler;
	}

	inputDimensions {
		^[[2]];
	}

	outputDimensions {
		^[[4]];
	}

	isSamplerVGen {
		^true;
	}
}
