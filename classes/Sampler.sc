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

	*pr { |image, pos|
		^this.multiNew(\pixel, pos).prSetupImageInput(image);
	}

	// Sets both U and V address modes at the same time.
	addressMode { |mode|
		addressModeU = mode;
		addressModeV = mode;
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
	*fr { |image|
		^this.multiNew(\frame).prSetupImageInput(image);
	}

	*sr { |image|
		^this.multiNew(\shape).prSetupImageInput(image);
	}

	*pr { |image|
		^this.multiNew(\pixel).prSetupImageInput(image);
	}

	inputDimensions {
		^[[]];
	}

	outputDimensions {
		^[[2]];
	}
}
