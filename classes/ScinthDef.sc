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
				vgen.inputs.do({ | input, inputIndex |
					case
					{ input.isNumber } {
						yaml = yaml ++ secondDepth ++ "- type: constant\n";
						yaml = yaml ++ secondDepth ++ "  value:" + input.asString ++ "\n";
					}
					{ input.isVGen } {
						yaml = yaml ++ secondDepth ++ "- type: vgen\n";
						yaml = yaml ++ secondDepth ++ "  vgenIndex:" + input.scinthIndex.asString ++ "\n";
						yaml = yaml ++ secondDepth ++ "  outputIndex: 0\n";
					}
					{ this.notImplemented; }
				});
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
	addVGen { | vGen |
		vGen.scinthIndex = children.size;
		children = children.add(vGen);
	}
}
