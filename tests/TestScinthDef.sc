TestScinthDef : UnitTest {
	// Validates every VGen in children has its index correct and references the containing ScinthDef.
	sanityCheckBuild { | def |
		this.assert(def.children.isArray);
		def.children.do({ | vgen, index |
			this.assert(vgen.isVGen);
			this.assertEquals(def, vgen.scinthDef);
			this.assertEquals(index, vgen.scinthIndex);
		});
	}

	// Validates the def against the yaml string by parsing the yaml and then checking the parsed
	// structure against the def.
	sanityCheckYAML { | def, yaml |
		var parse = yaml.parseYAML;

		// Top-level object is expected to be an array with a single entry, because each ScinthDef
		// YAML blob is an entry in a list.
		this.assert(parse.isArray);
		this.assertEquals(parse.size, 1);

		// Only entry in the top-level array should be a dictionary with at least name and vgens keys.
		this.assertEquals(parse[0].class, Dictionary);

		this.assertEquals(parse[0].at("name"), def.name.asString);

		this.assert(parse[0].at("vgens").isArray);
		this.assertEquals(parse[0].at("vgens").size, def.children.size);
		parse[0].at("vgens").do({ | vgen, index |
			this.assertEquals(vgen.class, Dictionary);
			this.assertEquals(vgen.at("class_name"), def.children[index].name);
			this.assertEquals(vgen.at("rate"), def.children[index].rate.asString);
			if (vgen.at("inputs").notNil, {
				this.assert(vgen.at("inputs").isArray);
				this.assertEquals(vgen.at("inputs").size, def.children[index].inputs.size);

				// Validate parsed inputs array elements.
				vgen.at("inputs").do({ | input, inputIndex |
					var defInput = def.children[index].inputs[inputIndex];
					this.assertEquals(input.class, Dictionary);
					switch(input.at("type"),
						"vgen", {
							this.assert(defInput.isVGen);
							this.assertEquals(input.at("vgen_index"), defInput.scinthIndex.asString);
							this.assertEquals(input.at("output_index"), "0");
						},
						"constant", {
							this.assert(defInput.isNumber);
							this.assertEquals(input.at("value"), defInput.asString);
						},
						{ this.assert(false, "unknown type in input dictionary"); }
					);
				});
			}, {
				this.assertEquals(def.children[index].inputs.size, 0);
			});
		});
	}

	// TODO: rename these test cases to indicate that they are all built with single-output VGens only,
	// so they aren't "linear chains" necessarily, more like trees instead of graphs (and therefore
	// need no topological sorting).

	test_single_vgen_build {
		var def = ScinthDef.new(\single_vgen_build, {
			VOut.fg(1.0);
		});

		this.sanityCheckBuild(def);

		// Def should have the correct name, a single child which is the correct type and
		// refers back to the def correctly, including having the correct index.
		this.assertEquals(def.name, \single_vgen_build);
		this.assertEquals(def.children.size, 1);
		this.assertEquals(def.children[0].class, VOut);
	}

	test_input_chain_linear_build {
		var def = ScinthDef.new(\input_chain_linear_build, {
			VOut.fg(VSinOsc.fg.abs);
		});

		this.sanityCheckBuild(def);

		this.assertEquals(def.name, \input_chain_linear_build);
		this.assertEquals(def.children.size, 3);
		this.assertEquals(def.children[0].class, VSinOsc);
		this.assertEquals(def.children[1].class, UnaryOpVGen);
		this.assertEquals(def.children[1].operator, \abs);
		this.assertEquals(def.children[1].inputs[0], def.children[0]);
		this.assertEquals(def.children[2].class, VOut);
	}

	test_single_vgen_yaml {
		var def = ScinthDef.new(\single_vgen_yaml, {
			VOut.fg(1.0.neg);
		});
		var yaml = def.asYAML;

		this.sanityCheckBuild(def);
		this.sanityCheckYAML(def, yaml);
	}

	test_input_chain_linear_yaml {
		var def = ScinthDef.new(\input_chain_linear_yaml, {
			VOut.fg(VSinOsc.fg(freq: ZPos.fg.neg, phase: XPos.fg, mul: 0.2, add: 0.5));
		});
		var yaml = def.asYAML;

		this.sanityCheckBuild(def);
		this.sanityCheckYAML(def, yaml);
	}
}
