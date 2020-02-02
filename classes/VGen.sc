VGen : AbstractFunction {
	classvar <>buildScinthDef;
	var <>scinthDef;
	var <>inputs;
	var <>rate = \fragment;
	var <>scinthIndex;
	var <>inDims;
	var <>outDims;

	*singleNew { | rate ... args |
		if (rate.isKindOf(Symbol).not or:
			{ rate != \fragment and: { rate != \vertex } }, {
				Error("rate must be one of 'fragment' or 'vertex'").throw;
		});
		^super.new.rate_(rate).addToScinth.init(*args);
	}

	*multiNew { |... args|
		^this.multiNewList(args)
	}

	*multiNewList { |argList|
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

	composeUnaryOp { |selector|
		^UnaryOpVGen.new(selector, this);
	}

	composeBinaryOp { |selector, input|
		if (input.isValidVGenInput, {
			^BinaryOpVGen.new(selector, this, input);
		}, {
			input.performBinaryOpOnVGen(selector, this);
		});
	}

	reverseComposeBinaryOp { |selector, vgen|
		^BinaryOpVGen.new(selector, vgen, this);
	}

	composeNAryOp {  |selector, argList|
		^thisMethod.notYetImplemented;
	}

	addToScinth {
		scinthDef = buildScinthDef;
		if (scinthDef.notNil, {
			scinthDef.addVGen(this);
		});
	}

	asVGenInput { ^this }
	isValidVGenInput { ^true }
	isVGen { ^true }
	name { ^this.class.asString }
	numOutputs { this.outputDimensions.length }

	inputDimensions {
		^[];
	}

	outputDimensions {
		^[];
	}
}

// These extensions mostly do the same as their UGen counterparts,
// but are maintained separately.

+ Object {
	asVGenInput { ^this }
	isValidVGenInput { ^false }
	isVGen { ^false }
}

+ AbstractFunction {
	asVGenInput { |for|
		^this.value(for);
	}

	isValidVGenInput { ^true }

	// could add some additional interesting ops ugens here, thinking
	// .dot, .cross, etc.
}

+ Array {
	asVGenInput { |for|
		^this.collect(_.asVGenInput(for));
	}

	isValidVGenInput { ^true }
}

+ SimpleNumber {
	isValidVGenInput { ^this.isNaN.not }
	inputDimensions { ^[] }
	outputDimensions { ^[[1]] }
}
