TestScinthDef : UnitTest {
	// restesting VGen build but now looking for references back to the Scindef and
	// indices consistent between individual VGens and their position in the children array.

	test_constant_vgen_build {
		var def = ScinthDef.new(\test1, {
			VSinOsc.fg(5.0, -1.0);
		});

		// Def should have the correct name, a single child which is the correct type and
		// refers back to the def correctly, including having the correct index.
		this.assertEquals(def.name, \test1);
		this.assert(def.children.isArray);
		this.assertEquals(def.children.size, 1);
		this.assertEquals(def.children[0].class, VSinOsc);
		this.assertEquals(def.children[0].scinthIndex, 0);
		this.assertEquals(def.children[0].scinthDef, def);
	}


	// Then YAML generation by generating YAML string, then parsing back to dicts and validating
	// contents.
}
