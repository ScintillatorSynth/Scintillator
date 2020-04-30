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
		newArgList = Array.newClear(argList.size);
		results = Array.newClear(size);
		size.do({ |i|
			argList.do({ |item, j|
				newArgList.put(j, if (item.class == Array, {
					item.wrapAt(i);
				}, {
					item;
				}));
			});
			results.put(i, this.multiNewList(newArgList));
		});
		^results;
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
	checkInputs { ^this.checkValidInputs; }
	checkValidInputs {
		inputs.do { | input, ind |
			if(input.isValidVGenInput.not) {
				^"arg: '%' has bad input: %".format(this.argNameForInputAt(ind), input);
			};
		};
		^nil;
	}
	isVGen { ^true }
	name { ^this.class.asString }
	numOutputs { this.outputDimensions.length }
	outputIndex { ^0 }

	inputDimensions {
		^[[]];
	}

	outputDimensions {
		^[[]];
	}

	isSamplerVGen {
		^false;
	}

	dumpArgs {
		" ARGS: ".postln;
		inputs.do { | input, ind |
			"     % : % %".format(this.argNameForInputAt(ind) ? ind.asString, input, input.class).postln;
		};
	}

	argNamesInputOffset { ^1; }

	argNameForInputAt { | ind |
		var method = this.class.class.findMethod(this.methodSelectorForRate);

		if(method.isNil || method.argNames.isNil) {
			^nil;
		};

		^method.argNames.at(ind + this.argNamesInputOffset);
	}

	methodSelectorForRate {
		^switch(rate)
		{\fragment} { \fg }
		{nil}
	}
}

MultiOutVGen : VGen {
	var <outputs;

	initOutputs { |numOutputs, rate|
		outputs = Array.fill(numOutputs, { |i|
			VOutputProxy.new(rate, this, i);
		});
		if (numOutputs == 1, {
			^outputs.at(0);
		});
		^outputs;
	}

	numOutputs { ^outputs.size }
	scinthIndex_ { |index|
		scinthIndex = index;
		outputs.do({ |output| output.scinthIndex = index; });
	}
}

VOutputProxy : VGen {
	var <>source;
	var <>outputIndex;

	*new { |rate, sourceVGen, index|
		^super.singleNew(rate, sourceVGen, index)
	}

	addToScinth {
		scinthDef = buildScinthDef;
	}

	init { |sourceVGen, index|
		source = sourceVGen;
		outputIndex = index;
		scinthIndex = source.scinthIndex;
	}

	isControlVGen {
		^source.isControlVGen;
	}
}

// These extensions mostly do the same as their UGen counterparts,
// but are maintained separately.

+ Object {
	asVGenInput { ^this }
	isValidVGenInput { ^false }
	isVGen { ^false }
	isControlVGen { ^false }
	isSamplerVGen { ^false }
}

+ AbstractFunction {
	asVGenInput { |for|
		^this.value(for);
	}

	isValidVGenInput { ^true }

	// could add some additional interesting ops vgens here, thinking
	// .dot, .cross, etc.
}

+ UGen {
	isValidVGenInput { ^false }
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
