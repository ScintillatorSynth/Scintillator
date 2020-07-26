TestScinthDef : UnitTest {
	classvar <>onFail;

	assert { |t, message|
		super.assert(t, message, onFailure: onFail);
	}

	assertEquals { |a, b, message = ""|
		super.assertEquals(a, b, message, onFailure: onFail);
	}

	// Validates every VGen in children has its index correct and references the containing ScinthDef.
	sanityCheckBuild { | def |
		this.assert(def.children.isArray);
		def.children.do({ | vgen, index |
			this.assert(vgen.isVGen);
			this.assertEquals(def, vgen.scinthDef);
			this.assertEquals(index, vgen.scinthIndex);
		});
		this.assert(def.controls.isArray);
		this.assert(def.controlNames.isArray);
		this.assertEquals(def.controls.size, def.controlNames.size);
	}

	// Validates the def against the yaml string by parsing the yaml and then checking the parsed
	// structure against the def.
	sanityCheckYAML { | def, yaml |
		var parse = yaml.parseYAML;

		// Top-level object is expected to be a Dictionary
		this.assertEquals(parse.class, Dictionary);
		this.assertEquals(parse.at("name"), def.name.asString);
		if (def.controls.size > 0, {
			var params = parse.at("parameters");
			this.assert(params.isArray);
			this.assertEquals(params.size, def.controls.size);
			params.do({ |paramDict, i|
				this.assertEquals(paramDict.class, Dictionary);
				this.assertEquals(paramDict["name"], def.controlNames[i].asString);
				this.assertEquals(paramDict["defaultValue"].asFloat, def.controls[i]);
			});
		}, {
			// No controls means param key should be absent.
			this.assert(parse.at("parameters").isNil);
		});

		this.assert(parse.at("vgens").isArray);
		this.assertEquals(parse.at("vgens").size, def.children.size);
		parse.at("vgens").do({ |vgen, index|
			this.assertEquals(vgen.class, Dictionary);
			this.assertEquals(vgen.at("className"), def.children[index].name.asString);
			this.assertEquals(vgen.at("rate"), def.children[index].rate.asString);
			if (vgen.at("inputs").notNil, {
				this.assert(vgen.at("inputs").isArray);
				this.assertEquals(vgen.at("inputs").size, def.children[index].inputs.size);

				// Validate parsed inputs array elements.
				vgen.at("inputs").do({ |input, inputIndex|
					var defInput = def.children[index].inputs[inputIndex];
					this.assertEquals(input.class, Dictionary);
					switch(input.at("type"),
						"vgen", {
							this.assert(defInput.isVGen);
							this.assertEquals(input.at("vgenIndex"), defInput.scinthIndex.asString);
							this.assertEquals(input.at("outputIndex"), "0");
						},
						"constant", {
							this.assert(defInput.isNumber);
							this.assertEquals(input.at("value"), defInput.asString);
						},
						"parameter", {
							this.assert(defInput.isControlVGen);
							this.assertEquals(input.at("index").asInteger, defInput.outputIndex);
						},
						{ this.assert(false, "unknown type in input dictionary"); }
					);
				});
			}, {
				this.assertEquals(def.children[index].inputs.size, 0);
			});
		});
	}

	test_singleVgenBuild {
		var def = ScinthDef.new(\singleVgenBuild, {
			VBWOut.pr(1.0);
		});

		this.sanityCheckBuild(def);

		// Def should have the correct name, a single child which is the correct type and
		// refers back to the def correctly, including having the correct index. Also
		// no parameters for this def so the controls array should be empty.
		this.assertEquals(def.name, \singleVgenBuild);
		this.assertEquals(def.children.size, 1);
		this.assertEquals(def.children[0].class, VBWOut);
		this.assertEquals(def.controls.size, 0);
	}

	test_inputChainLinearBuild {
		var def = ScinthDef.new(\inputChainLinearBuild, {
			VBWOut.pr(VSinOsc.pr.abs);
		});

		this.sanityCheckBuild(def);

		this.assertEquals(def.name, \inputChainLinearBuild);
		this.assertEquals(def.children.size, 3);
		this.assertEquals(def.children[0].class, VSinOsc);
		this.assertEquals(def.children[1].class, UnaryOpVGen);
		this.assertEquals(def.children[1].name, 'VAbs');
		this.assertEquals(def.children[1].inputs[0], def.children[0]);
		this.assertEquals(def.children[2].class, VBWOut);
		this.assertEquals(def.controls.size, 0);
	}

	test_singleVgenYaml {
		var def = ScinthDef.new(\singleVgenYaml, {
			VBWOut.pr(1.0.neg);
		});
		var yaml = def.asYAML;

		this.sanityCheckBuild(def);
		this.sanityCheckYAML(def, yaml);
	}

	test_inputChainYaml {
		var def = ScinthDef.new(\inputChainYaml, {
			var pos = VNormPos.pr;
			VBWOut.pr(VSinOsc.pr(freq: VY.pr(pos).neg, phase: VX.pr(pos), mul: 0.2, add: 0.5));
		});
		var yaml = def.asYAML;

		this.sanityCheckBuild(def);
		this.sanityCheckYAML(def, yaml);
	}

	test_paramsNoValues {
		var def = ScinthDef.new(\paramsNoValues, { |a, b, c|
			var sin3 = VVec3.pr(a, b, c).sin;
			VRGBOut.pr(VX.pr(sin3), VY.pr(sin3), VZ.pr(sin3));
		});
		// Make sure names are in correct order and all represented.
		this.assertEquals(3, def.controlNames.size);
		this.assertEquals('a', def.controlNames[0]);
		this.assertEquals('b', def.controlNames[1]);
		this.assertEquals('c', def.controlNames[2]);
		// Defaults should all be zeros for parameters that don't specify a default
		this.assertEquals(3, def.controls.size);
		this.assertEquals(0, def.controls[0]);
		this.assertEquals(0, def.controls[1]);
		this.assertEquals(0, def.controls[2]);

		this.sanityCheckBuild(def);
		this.sanityCheckYAML(def, def.asYAML);
	}

	test_paramsMixedValues {
		var def = ScinthDef.new(\paramsValues, { |quick = 10.0, slow = 0.001, x = -17|
			var average = quick + slow / 2;
			VBWOut.pr(average - x);
		});
		this.assertEquals(3, def.controlNames.size);
		this.assertEquals('quick', def.controlNames[0]);
		this.assertEquals('slow', def.controlNames[1]);
		this.assertEquals('x', def.controlNames[2]);
		this.assertEquals(3, def.controls.size);
		this.assertEquals(10.0, def.controls[0]);
		this.assertEquals(0.001, def.controls[1]);
		this.assertEquals(-17, def.controls[2]);

		this.sanityCheckBuild(def);
		this.sanityCheckYAML(def, def.asYAML);
	}

	test_vgenInputValid_nilIsInvalid {
		var expected = "VBWOut arg: 'value' has bad input: nil";
		var err;

		try {
			ScinthDef(\vgen_nilIsInvalid, {
				VBWOut.pr(nil);
			});
		} { | e |
			err = e;
		};

		this.assert(err != nil, "VGen threw an error");
		this.assert(expected == err.what, "VGen threw the expected error");
	}

	test_vgenInputValid_ugenIsInvalid {
		var expected = "VBWOut arg: 'value' has bad input: a SinOsc";
		var err;

		try {
			ScinthDef(\vgen_nilIsInvalid, {
				VBWOut.pr(SinOsc.ar());
			});
		} { | e |
			err = e;
		};

		this.assert(err != nil, "VGen threw an error");
		this.assert(expected == err.what, "VGen threw the expected error");
	}
}
