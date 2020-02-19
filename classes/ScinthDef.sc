ScinthDef {
	var <>name;
	var <>func;
	var <>children;
	var <>defServer;
	var <>controls;
	var <>controlNames;

	*new { |name, vGenGraphFunc|
		^super.newCopyArgs(name.asSymbol).children_(Array.new(64)).build(vGenGraphFunc);
	}

	build { |vGenGraphFunc|
		VGen.buildScinthDef = this;
		func = vGenGraphFunc;
		func.valueArray(this.prBuildControls);
		VGen.buildScinthDef = nil;
		children.do({ |vgen, index| this.prFitDimensions(vgen) });
		^this;
	}

	// All controls right now are at the fragment rate, so no grouping or sorting is needed, and we only
	// create at most one element in the controls member Array
	prBuildControls {
		controlNames = func.def.argNames;
		if (controlNames.size >= 32, {
			Error.new("Scintillator ScinthDes support a maximum of 32 arguments.").throw;
		});
		if (controlNames.isNil, {
			controls = [];
			controlNames = [];
		}, {
			controls = func.def.prototypeFrame.extend(controlNames.size);
			controls = controls.collect({ |value| value ? 0.0 });
			^VControl.fg(controls)
		});
	}

	prFitDimensions { |vgen|
		var dimIndex = -1;
		// Build list of input dimensions.
		vgen.inDims = Array.fill(vgen.inputs.size, { |i|
			case
			{ vgen.inputs[i].isNumber } { 1 }
			{ vgen.inputs[i].isControlVGen } { 1 }
			{ vgen.inputs[i].isVGen } { children[vgen.inputs[i].scinthIndex].outDims[0] }
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
			"dimension mismatch on vgen % in ScinthDef %".format(vgen.name, name).postln;
		});
	}

	asYAML { |indentDepth = 0|
		var indent, depthIndent, secondDepth, yaml;
		indent = "";
		indentDepth.do({ indent = indent ++ "    " });
		depthIndent = indent ++ "    ";
		secondDepth = depthIndent ++ "    ";

		yaml = indent ++ "name: %\n".format(name);
		if (controls.size > 0, {
			yaml = yaml ++ indent ++ "parameters:\n";
			controls.do({ |control, i|
				yaml = yaml ++ depthIndent ++ "- name: " ++ controlNames[i] ++ "\n";
				yaml = yaml ++ depthIndent ++ "  defaultValue: " ++ control.asString ++ "\n";
			});
		});
		yaml = yaml ++ indent ++ "vgens:\n";
		children.do({ | vgen, index |
			yaml = yaml ++ depthIndent ++ "- className:"  + vgen.name ++ "\n";
			yaml = yaml ++ depthIndent ++ "  rate: fragment\n";
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
						if (vgen.inputValueType(inputIndex) != \float, {
							Error.new("Non-float input types don't support VGens").throw;
						});
						yaml = yaml ++ secondDepth ++ "- type: vgen\n";
						yaml = yaml ++ secondDepth ++ "  vgenIndex:" + input.scinthIndex.asString ++ "\n";
						yaml = yaml ++ secondDepth ++ "  outputIndex: 0\n";
						yaml = yaml ++ secondDepth ++ "  dimension:" + vgen.inDims[inputIndex] ++ "\n";
					}
					{ Error.new("unknown input").throw };
					yaml = yaml ++ secondDepth ++ "  valueType:" + vgen.inputValueType(inputIndex) ++ "\n";
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

}
