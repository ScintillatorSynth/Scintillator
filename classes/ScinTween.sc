ScinTween {
	classvar <tweenNames;

	var <levels, <times, <curves, <sampleRate;
	var <>tweenIndex;

	*initClass {
		// These values need to match those specified in Tween.hpp in scinserver.
		tweenNames = IdentityDictionary.newFrom(
			(
				backIn: 0,
				backInOut: 1,
				bounceIn: 2,
				bounceInOut: 3,
				bounceOut: 4,
				circularIn: 5,
				circularInOut: 6,
				circularOut: 7,
				cubicIn: 8,
				cubicInOut: 9,
				cubicOut: 10,
				elasticIn: 11,
				elasticInOut: 12,
				elasticOut: 13,
				exponentialIn: 14,
				exponentialInOut: 15,
				exponentialOut: 16,
				linear: 17,
				quadraticIn: 18,
				quadraticInOut: 19,
				quadraticOut: 20,
				quarticIn: 21,
				quarticInOut: 22,
				quarticOut: 23,
				quinticIn: 24,
				quinticInOut: 25,
				quinticOut: 26,
				sinusodalIn: 27,
				sinusodalInOut: 28,
				sinusodalOut: 29
			)
		);

	}

	*new { |levels = #[0, 1, 0], times = #[1, 1], curves=\linear, sampleRate = 120|
		times = times.asArray.wrapExtend(levels.size - 1);
		^super.newCopyArgs(levels, times, curves, sampleRate);
	}

	levelDimension {
		// We use the first element to determine the dimension of all of the levels.
		if (levels[0].isNumber, { ^1 }, { ^levels[0].size; });
	}

	isValidVGenInput { ^true }
	asVGenInput { ^this }
}

VTweenGen : VGen {
	var <>tween;

	*fr { |tween, levelScale = 1.0, levelBias = 0.0, timeScale = 1.0|
		VTweenGen.prAddTween(tween);
		^this.multiNew(\frame, tween, levelScale, levelBias, timeScale).tween_(tween);
	}

	*sr { |tween, levelScale = 1.0, levelBias = 0.0, timeScale = 1.0|
		VTweenGen.prAddTween(tween);
		^this.multiNew(\shape, tween, levelScale, levelBias, timeScale).tween_(tween);
	}

	*pr { |tween, levelScale = 1.0, levelBias = 0.0, timeScale = 1.0|
		VTweenGen.prAddTween(tween);
		^this.multiNew(\pixel, tween, levelScale, levelBias, timeScale).tween_(tween);
	}

	*prAddTween { |tween|
		if (tween.class.asSymbol !== 'ScinTween', {
			Error.new("First argument to VTweenGen must be a ScinTween.").throw;
		});
		if (tween.tweenIndex.isNil, {
			VGen.buildScinthDef.tweens.add(tween);
			tween.tweenIndex = VGen.buildScinthDef.tweens.size - 1;
		});
	}

	inputDimensions {
		^[[1, 1, 1, 1]];
	}

	outputDimensions {
		^[[tween.levelDimension]];
	}
}