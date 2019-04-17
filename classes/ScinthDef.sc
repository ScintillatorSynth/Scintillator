ScinthDef {
	var <>name;
	var <>func;
	var <>children;

	*new { | name, vGenGraphFunc |
		^super.newCopyArgs(name.asSymbol)
		.children_(Array.new(64))
		.build(vGenGraphFunc);
	}

	build { | vGenGraphFunc |
		VGen.buildScinthDef = this;
		func = vGenGraphFunc;
		func.valueArray();
		VGen.buildScinthDef = nil;
	}

	asYAML { | indentDepth = 1 |
		var yaml, indent, depthIndent, secondDepth;
		indentDepth.do({ indent = indent ++ "    " });
		yaml = indent ++ "- name: %\n".format(name);
		yaml = yaml ++ indent ++ "  vgens:\n";
		depthIndent = indent ++ "    ";
		secondDepth = depthIndent ++ "    ";
		children.do({ | vgen, index |
			yaml = yaml ++ depthIndent ++ "- class_name:"  + vgen.name ++ "\n";
			yaml = yaml ++ depthIndent ++ "  rate: fragment\n";
			yaml = yaml ++ depthIndent ++ "  inputs:\n";
			vgen.inputs.do({ | input, inputIndex |
				case
				{ input.isNumber } {
					yaml = yaml ++ secondDepth ++ "- type: constant\n";
					yaml = yaml ++ secondDepth ++ "  value:" + input.asString ++ "\n";
				}
				{ input.isVGen } {
					yaml = yaml ++ secondDepth ++ "- type: vgen\n";
					yaml = yaml ++ secondDepth ++
					"  vgen_index:" + input.scinthIndex.asString ++ "\n";
					yaml = yaml ++ secondDepth ++ "  output_index: 0\n";
				}
			});
		});

		^yaml;
	}

	// VGens call these.
	addVGen { | vGen |
		vGen.scinthIndex = children.size;
		children = children.add(vGen);
	}
}
