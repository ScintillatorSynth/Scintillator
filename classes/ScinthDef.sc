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

	asYAML { | indent = 4 |
		var indentString, yamlString, depthIndent, secondDepth;
		indent.do({ indentString = indentString ++ " " });
		yamlString = indentString ++ "- name: %\n".format(name);
		yamlString = yamlString ++ indentString ++ "  vgens:\n";
		depthIndent = indentString ++ "    ";
		secondDepth = depthIndent ++ "    ";
		children.do({ | vgen, index |
			yamlString = yamlString ++ depthIndent ++ "- class_name:"  + vgen.class.asString ++ "\n";
			yamlString = yamlString ++ depthIndent ++ "  rate: fragment\n";
			yamlString = yamlString ++ depthIndent ++ "  inputs:\n";
			vgen.inputs.do({ | input, inputIndex |
				case
				{ input.isNumber } {
					yamlString = yamlString ++ secondDepth ++ "- type: constant\n";
					yamlString = yamlString ++ secondDepth ++ "  value:" + input.asString ++ "\n";
				}
				{ input.isVGen } {
					yamlString = yamlString ++ secondDepth ++ "- type: vgen\n";
					yamlString = yamlString ++ secondDepth ++
					"  vgen_index:" + input.scinthIndex.asString ++ "\n";
					yamlString = yamlString ++ secondDepth ++ "  output_index: 0\n";
				}
			});
		});

		^yamlString;
	}

	// VGens call these.
	addVGen { | vGen |
		vGen.scinthIndex = children.size;
		children = children.add(vGen);
	}
}
