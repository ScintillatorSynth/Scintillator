VGen : AbstractFunction {
	classvar <>buildScinthDef;
	var <>scinthDef;
	var <>inputs;
	var <>rate = \fragment;
	var <>scinthIndex;

	*singleNew { | rate ... args |
		if (rate.isKindOf(Symbol).not or:
			{ rate != \fragment and: { rate != \vertex } }, {
				Error("rate must be one of 'fragment' or 'vertex'").throw;
		});
		^super.new.rate_(rate).addToSynth.init(*args);
	}

	*multiNew { | ... args |
		^this.multiNewList(args)
	}

	*multiNewList { | argList |
		var size = 0, newArgList, results;

		// Flatten AbstractFunction descendants in argument list to VGens.
		argList = argList.asVGenInput(this);

		// Identify longest array in argument list.
		argList.do({ | item |
			if (item.class == Array, {
				size = max(size, item.size);
			});
		});

		// If no arrays found, no multichannel expansion happening,
		// can process as single new object in chain.
		if (size == 0, {
			^this.singleNew(*argList);
		});

		// TODO: test multichannel, to understand this.
		newArgList = Array.newClear(argList, size);
		results = Array.newClear(size);
		size.do({ | i |
			argList.do({ | item, j |
				newArgList.put(j, if (item.class == Array, {
					item.wrapAt(i);
				}, {
					item;
				}));
			});
			results.put(i, this.multiNewList(newArgList));
		});
	}

	init { | ... theInputs |
		inputs = theInputs;
	}

	composeUnaryOp { | selector |
		^UnaryOpVGen.new(selector, this);
	}

	composeBinaryOp { | selector, input |
		if (input.isValidVGenInput, {
			^BinaryOpVGen.new(selector, this, input);
		}, {
			input.performBinaryOpOnVGen(selector, this);
		});
	}

	reverseComposeBinaryOp { | selector, something, adverb |
		^thisMethod.notYetImplemented;
	}

	composeNAryOp {  | selector, argList |
		^thisMethod.notYetImplemented;
	}

	addToSynth {
		scinthDef = buildScinthDef;
		if (scinthDef.notNil, {
			scinthDef.addVGen(this);
		});
	}

	asVGenInput { ^this }
	isValidVGenInput { ^true }
	isVGen { ^true }
	name { ^this.class.asString }
	numOutputs { ^1 }

	madd { | mul = 1.0, add = 0.0 |
		^VMulAdd.new(this, mul, add);
	}
}

// No side effects in PureVGens, placeholder for now but subject
// to graph optimizations (?) in the future.
PureVGen : VGen {

}

// These extensions mostly do the same as their UGen counterparts,
// but are maintained separately.

+ Object {
	asVGenInput { ^this }
	isValidVGenInput { ^false }
	isVGen { ^false }
}

+ AbstractFunction {
	asVGenInput { | for |
		^this.value(for);
	}

	isValidVGenInput { ^true }
}

+ Array {
	asVGenInput { | for |
		^this.collect(_.asVGenInput(for));
	}

	isValidVGenInput { ^true }
}

+ SimpleNumber {
	isValidVGenInput { ^this.isNaN.not }
}
