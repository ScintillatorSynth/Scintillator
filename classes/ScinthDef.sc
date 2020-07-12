ScinthDef {
	var <>name;
	var <>func;
	var <>shape;
	var <>renderOptions;
	var <>children;
	var <>defServer;
	var <>controls;
	var <>controlNames;

	*new { |name, vGenGraphFunc, shape, renderOptions|
		^super.newCopyArgs(name.asSymbol, vGenGraphFunc, shape, renderOptions).children_(Array.new(64)).build();
	}

	build {
		if (shape.isNil, {
			shape = Quad.new;
		}, {
			if (shape.isShape.not, {
				Error.new("Non-shape object provided as Shape argument.").throw;
			});
		});
		// renderOptions as nil just means we accept the default

		VGen.buildScinthDef = this;
		func.valueArray(this.prBuildControls);

		protect {
			this.checkInputs;
			children.do({ |vgen, index| this.prFitDimensions(vgen) });
			this.prCheckRates(children.wrapAt(-1), \pixel);
		} {
			VGen.buildScinthDef = nil;
		}
	}

	prBuildControls {
		controlNames = func.def.argNames;
		if (controlNames.size >= 32, {
			Error.new("Scintillator ScinthDef supports a maximum of 32 arguments.").throw;
		});
		if (controlNames.isNil, {
			controls = [];
			controlNames = [];
		}, {
			controls = func.def.prototypeFrame.extend(controlNames.size);
			controls = controls.collect({ |value| value ? 0.0 });
			^VControl.new(controls)
		});
	}

	prFitDimensions { |vgen|
		var dimIndex = -1;
		// Build list of input dimensions.
		vgen.inDims = Array.fill(vgen.inputs.size, { |i|
			case
			{ vgen.inputs[i].isNumber } { 1 }
			{ vgen.inputs[i].isControlVGen } { 1 }
			{ vgen.inputs[i].isVGen } { children[vgen.inputs[i].scinthIndex].outDims[vgen.inputs[i].outputIndex] }
			{ "*** vgen input class: %".format(vgen.inputs[i]).postln; nil; }
		});

		// Search for dimensions among list of supported input dimensions.
		vgen.inputDimensions.do({ |dim, i|
			if (dim == vgen.inDims, {
				dimIndex = i;
			});
		});

		if (dimIndex >= 0, {
			vgen.outDims = vgen.outputDimensions[dimIndex];
		}, {
			Error.new("Dimension mismatch on vgen % in ScinthDef %".format(vgen.name, name)).throw;
		});
	}

	// Starting from output VGen and recursing back to inputs check that rate only decreases or stays the same, meaning
	// that in opposite direction, from input to output, rate only increases or stays the same.
	prCheckRates { |vgen, rate|
		switch (rate,
			\frame, {
				if (vgen.rate !== \frame and: { vgen.rate !== \param }, {
					Error.new("Frame rate VGen can only accept other frame rate VGens as input, got a rate of %"
						.format(vgen.rate)).throw;
				});
			},
			\shape, {
				if (vgen.rate === \pixel, {
					Error.new("Shape rate VGen can only accept input from shape or frame rate VGens.").throw;
				});
			},
			// Pixel rate VGens accept input from any rate VGen.
			{});
		vgen.inputs.do({ |input|
			if (input.isVGen, {
				this.prCheckRates(input, vgen.rate);
			});
		});
	}

	asYAML { |indentDepth = 0|
		var indent, depthIndent, secondDepth, yaml;
		indent = "";
		indentDepth.do({ indent = indent ++ "    " });
		depthIndent = indent ++ "    ";
		secondDepth = depthIndent ++ "    ";

		yaml = indent ++ "name: %\n".format(name);

		yaml = yaml ++ indent ++ "shape:\n";
		yaml = yaml ++ shape.asYAML(depthIndent);

		if (renderOptions.notNil, {
			yaml = yaml ++ indent ++ "options:\n";
			renderOptions.keysValuesDo({ |key, value|
				yaml = yaml ++ depthIndent ++ "%: %\n".format(key, value);
			});
		});

		if (controls.size > 0, {
			yaml = yaml ++ indent ++ "parameters:\n";
			controls.do({ |control, i|
				yaml = yaml ++ depthIndent ++ "- name: " ++ controlNames[i] ++ "\n";
				yaml = yaml ++ depthIndent ++ "  defaultValue: " ++ control.asString ++ "\n";
			});
		});
		yaml = yaml ++ indent ++ "vgens:\n";
		children.do({ |vgen, index|
			yaml = yaml ++ depthIndent ++ "- className:"  + vgen.name ++ "\n";
			yaml = yaml ++ depthIndent ++ "  rate:" + vgen.rate ++ "\n";
			if (vgen.isSamplerVGen, {
				yaml = yaml ++ depthIndent ++ "  sampler:\n";
				yaml = yaml ++ secondDepth ++ "  image:" + vgen.image ++ "\n";
				yaml = yaml ++ secondDepth ++ "  imageArgType:" + vgen.imageArgType ++ "\n";
				yaml = yaml ++ secondDepth ++ "  minFilterMode:" + vgen.minFilterMode ++ "\n";
				yaml = yaml ++ secondDepth ++ "  magFilterMode:" + vgen.magFilterMode ++ "\n";
				yaml = yaml ++ secondDepth ++ "  enableAnisotropicFiltering:" + vgen.enableAnisotropicFiltering.asString + "\n";
				yaml = yaml ++ secondDepth ++ "  addressModeU:" + vgen.addressModeU ++ "\n";
				yaml = yaml ++ secondDepth ++ "  addressModeV:" + vgen.addressModeV ++ "\n";
				yaml = yaml ++ secondDepth ++ "  clampBorderColor:" + vgen.clampBorderColor ++ "\n";
			});
			if (vgen.inputs.size > 0, {
				yaml = yaml ++ depthIndent ++ "  inputs:\n";
				vgen.inputs.do({ |input, inputIndex|
					case
					{ input.isNumber } {
						yaml = yaml ++ secondDepth ++ "- type: constant\n";
						yaml = yaml ++ secondDepth ++ "  dimension:" + vgen.inDims[inputIndex] ++ "\n";
						yaml = yaml ++ secondDepth ++ "  value:" + input.asString ++ "\n";
					}
					{ input.isControlVGen } {
						yaml = yaml ++ secondDepth ++ "- type: parameter\n";
						yaml = yaml ++ secondDepth ++ "  index: " + input.outputIndex ++ "\n";
						yaml = yaml ++ secondDepth ++ "  dimension: 1\n";
					}
					{ input.isVGen } {
						yaml = yaml ++ secondDepth ++ "- type: vgen\n";
						yaml = yaml ++ secondDepth ++ "  vgenIndex:" + input.scinthIndex.asString ++ "\n";
						yaml = yaml ++ secondDepth ++ "  outputIndex: 0\n";
						yaml = yaml ++ secondDepth ++ "  dimension:" + vgen.inDims[inputIndex] ++ "\n";
					}
					{ Error.new("unknown input").throw };
				});
			});
			yaml = yaml ++ depthIndent ++ "  outputs:\n";
			vgen.outDims.do({ |outDim, outDimIndex|
				yaml = yaml ++ secondDepth ++ "- dimension:" + outDim ++ "\n";
			});
		});

		^yaml;
	}

	add { |server, completionMsg|
		if (server.isNil, {
			server = ScinServer.default;
		});
		defServer = server;
		server.sendMsg('/scin_d_recv', this.asYAML, completionMsg);
	}

	free {
		defServer.sendMsg('/scin_d_free', name);
	}

	// VGens call these.
	addVGen { |vgen|
		vgen.scinthIndex = children.size;
		children = children.add(vgen);
	}

	checkInputs {
		var firstErr;
		children.do { | vgen |
			var err;
			if((err = vgen.checkInputs).notNil) {
				err = vgen.class.asString + err;
				err.postln;
				vgen.dumpArgs;

				if(firstErr.isNil) {
					firstErr = err;
				};
			};
		};

		if(firstErr.notNil) {
			"ScinthDef % build failed".format(this.name).postln;
			Error(firstErr).throw;
		};

		^true;
	}

}
