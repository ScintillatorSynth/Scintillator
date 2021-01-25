ScinTween {
	classvar <tweenNames;

	var <levels, <times, <curves, <sampleRate, <loop;
	var <>tweenIndex;

	*initClass {
		// These values need to match those specified in Tween.hpp in scinserver.
		tweenNames = IdentityDictionary.newFrom(
			(
				backIn: 0,
				backInOut: 1,
				backOut: 2,
				bounceIn: 3,
				bounceInOut: 4,
				bounceOut: 5,
				circularIn: 6,
				circularInOut: 7,
				circularOut: 8,
				cubicIn: 9,
				cubicInOut: 10,
				cubicOut: 11,
				elasticIn: 12,
				elasticInOut: 13,
				elasticOut: 14,
				exponentialIn: 15,
				exponentialInOut: 16,
				exponentialOut: 17,
				linear: 18,
				quadraticIn: 19,
				quadraticInOut: 20,
				quadraticOut: 21,
				quarticIn: 22,
				quarticInOut: 23,
				quarticOut: 24,
				quinticIn: 25,
				quinticInOut: 26,
				quinticOut: 27,
				sinusoidalIn: 28,
				sinusoidalInOut: 29,
				sinusoidalOut: 30
			)
		);

	}

	*new { |levels = #[0, 1], times = #[1], curves=\linear, sampleRate = 120, loop = false|
		times = times.asArray.wrapExtend(levels.size - 1);
		^super.newCopyArgs(levels, times, curves, sampleRate, loop);
	}

	levelDimension {
		// We use the first element to determine the dimension of all of the levels.
		if (levels[0].isNumber, { ^1 }, { ^levels[0].size; });
	}

	isValidVGenInput { ^true }
	asVGenInput { ^this }
}

BaseVTweenGen : VGen {
	var <>tween;

	*fr { |tween, levelScale = 1.0, levelBias = 0.0, timeScale = 1.0|
		BaseVTweenGen.prAddTween(tween);
		^this.multiNew(\frame, levelScale, levelBias, timeScale).tween_(tween);
	}

	*sr { |tween, levelScale = 1.0, levelBias = 0.0, timeScale = 1.0|
		BaseVTweenGen.prAddTween(tween);
		^this.multiNew(\shape, levelScale, levelBias, timeScale).tween_(tween);
	}

	*pr { |tween, levelScale = 1.0, levelBias = 0.0, timeScale = 1.0|
		BaseVTweenGen.prAddTween(tween);
		^this.multiNew(\pixel, levelScale, levelBias, timeScale).tween_(tween);
	}

	*prAddTween { |tween|
		if (tween.class.asSymbol !== 'ScinTween', {
			Error.new("First argument to VTweenGen must be a ScinTween.").throw;
		});
		if (tween.levelDimension == 3, {
			Error.new("Due to limited hardware support, 3D Tweens are not supported.").throw;
		});
		if (tween.tweenIndex.isNil, {
			VGen.buildScinthDef.tweens.add(tween);
			tween.tweenIndex = VGen.buildScinthDef.tweens.size - 1;
		});
	}

	hasTweenVGen { ^true }

	inputDimensions {
		^[[1, 1, 1]];
	}
}

VTweenGen1 : BaseVTweenGen {
	outputDimensions {
		^[[1]];
	}
}

VTweenGen2 : BaseVTweenGen {
	outputDimensions {
		^[[2]];
	}
}

VTweenGen4 : BaseVTweenGen {
	outputDimensions {
		^[[4]];
	}
}

BaseVTweenSampler : VGen {
	var <>tween;

	*fr { |tween, t|
		BaseVTweenSampler.prAddTween(tween);
		^this.multiNew(\frame, t).tween_(tween);
	}

	*sr { |tween, t|
		BaseVTweenSampler.prAddTween(tween);
		^this.multiNew(\shape, t).tween_(tween);
	}

	*pr { |tween, t|
		BaseVTweenSampler.prAddTween(tween);
		^this.multiNew(\pixel, t).tween_(tween);
	}

	*prAddTween { |tween|
		if (tween.class.asSymbol !== 'ScinTween', {
			Error.new("First argument to VTweenSampler must be a ScinTween.").throw;
		});
		if (tween.levelDimension == 3, {
			Error.new("Due to limited hardware support, 3D Tweens are not supported.").throw;
		});
		if (tween.tweenIndex.isNil, {
			VGen.buildScinthDef.tweens.add(tween);
			tween.tweenIndex = VGen.buildScinthDef.tweens.size - 1;
		});
	}

	hasTweenVGen { ^true }

	inputDimensions {
		^[[1]];
	}
}

VTweenSampler1 : BaseVTweenSampler {
	outputDimensions {
		^[[1]];
	}
}

VTweenSampler2 : BaseVTweenSampler {
	outputDimensions {
		^[[2]];
	}
}

VTweenSampler4 : BaseVTweenSampler {
	outputDimensions {
		^[[4]];
	}
}
