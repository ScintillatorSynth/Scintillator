ScinthDef {
	var <>name;
	var <>func;
	var <>children;

	*new { | name, vGenGraphFunc |
		^super.newCopyArgs(name.asSymbol).children_(Array.new(64)).build(vGenGraphFunc);
	}

	build { | vGenGraphFunc |
		VGen.buildScinthDef = this;
		func = vGenGraphFunc;
		func.valueArray();
		VGen.buildScinthDef = nil;
		children.do({ |vgen, index| this.prFitDimensions(vgen) });
	}

	prFitDimensions { |vgen|
		var dimIndex = -1;
		// Build list of input dimensions.
		vgen.inDims = Array.fill(vgen.inputs.size, { |i|
			case
			{ vgen.inputs[i].isNumber } { 1 }
			{ vgen.inputs[i].isVGen } { children[vgen.inputs[i].scinthIndex].outDims[0] }
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
		yaml = indent ++ "- name: %\n".format(name);
		yaml = yaml ++ indent ++ "  vgens:\n";
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
					{ input.isVGen } {
						yaml = yaml ++ secondDepth ++ "- type: vgen\n";
						yaml = yaml ++ secondDepth ++ "  vgenIndex:" + input.scinthIndex.asString ++ "\n";
						yaml = yaml ++ secondDepth ++ "  outputIndex: 0\n";
						yaml = yaml ++ secondDepth ++ "  dimension:" + vgen.inDims[inputIndex] ++ "\n";
					}
					{ this.notImplemented; }
				});
			});
			yaml = yaml ++ depthIndent ++ "  outputs:\n";
			vgen.outDims.do({ |outDim, outDimIndex|
				yaml = yaml ++ secondDepth ++ "- dimension:" + outDim ++ "\n";
			});
		});

		^yaml;
	}

	// Ultimately this should find the currently attached Scintillator Server, serialize it and send it
	// to the server. But for now, it completes the YAML file spec and saves it to disk, for loading
	// offline by the alpha server.
	add { |filePath|
		var file;
		var yaml = "---\n";
		yaml = yaml ++ this.asYAML;
		file = File(filePath, "w");
		file.write(yaml);
		file.close;
	}

	// VGens call these.
	addVGen { |vgen|
		vgen.scinthIndex = children.size;
		children = children.add(vgen);
	}
}
