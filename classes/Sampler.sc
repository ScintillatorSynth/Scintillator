// Base class for any VGen that samples pixel values outside of its own local value.
Sampler : VGen {
	var <>image;
	// one of \constant, \parameter.
	var <>imageArgType;

	// one of \linear, \nearest.
	var <>minFilterMode = \linear;
	var <>magFilterMode = \linear;

	var <>enableAnisotropicFiltering = true;

	// one of \repeat, \mirroredRepeat, \clampToEdge, or \clampToBorder
	var <>addressModeU = \clampToBorder;
	var <>addressModeV = \clampToBorder;

	// one of \transparentBlack, \black, \white, only used if addressMode is set to \clampToBorder.
	var <>clampBorderColor = \transparentBlack;

	*fg { |image, pos|
		var sampler = this.multiNew(\fragment, pos);
		image = image.asVGenInput;
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
