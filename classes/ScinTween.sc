ScinTween {
	var <levels, <times, <curve, <sampleRate;
	var <>tweenIndex;

	*new { |levels = #[0, 1, 0], times = #[1, 1], curve=\lin, sampleRate = 120|
		times = times.asArray.wrapExtend(levels.size - 1);
		^super.newCopyArgs(levels, times, curve, sampleRate);
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