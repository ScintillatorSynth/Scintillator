ScinthDef {
	var <>name;
	var <>func;
	var <>children;
	var <>defServer;
	var <>controls;  // currently an array with one or no elements, a VControl object

	*new { |name, vGenGraphFunc|
		^super.newCopyArgs(name.asSymbol).children_(Array.new(64)).build(vGenGraphFunc);
	}

	build { |vGenGraphFunc|
		VGen.buildScinthDef = this;
		func = vGenGraphFunc;
		this.prBuildControls;
		func.valueArray(controls);
		VGen.buildScinthDef = nil;
		children.do({ |vgen, index| this.prFitDimensions(vgen) });
		^this;
	}

	// All controls right now are at the fragment rate, so no grouping or sorting is needed, and we only
	// create at most one element in the controls member Array
	prBuildControls {
		var values;
		var names = func.def.argNames;
		if (names.isNil, { ^nil });
		values = func.def.prototypeFrame.extend(names.size);
		values = values.collect({ |value| value ? 0.0 });
		^VControl.fg(values);
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
		var yaml, indent, depthIndent, secondDepth;
		indent = "";
		indentDepth.do({ indent = indent ++ "    " });
		yaml = indent ++ "name: %\n".format(name);
		yaml = yaml ++ indent ++ "vgens:\n";
		depthIndent = indent ++ "    ";
		secondDepth = depthIndent ++ "    ";
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
						"*** control yaml output".postln;
					}
					{ input.isVGen } {
						yaml = yaml ++ secondDepth ++ "- type: vgen\n";
						yaml = yaml ++ secondDepth ++ "  vgenIndex:" + input.scinthIndex.asString ++ "\n";
						yaml = yaml ++ secondDepth ++ "  outputIndex: 0\n";
						yaml = yaml ++ secondDepth ++ "  dimension:" + vgen.inDims[inputIndex] ++ "\n";
					}
					{ thisMethod.notImplemented }
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
