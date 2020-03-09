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
		^this.multiNew(\fragment, pos).prSetupImageInput(image);
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

	prSetupImageInput { |inImage|
		inImage = inImage.asVGenInput;
		case
		{ inImage.isNumber } {
			image = inImage;
			imageArgType = \constant;
		}
		{ inImage.isControlVGen } {
			image = inImage.outputIndex;
			imageArgType = \parameter;
		}
		{ Error.new("Unsupported argument type to Sampler image argument. Supported types are constants and parameters.").throw; };
	}
}

TextureSize : Sampler {
	*fg { |image|
		^this.multiNew(\fragment).prSetupImageInput(image);
	}

	inputDimensions {
		^[[]];
	}

	outputDimensions {
		^[[2]];
	}
}