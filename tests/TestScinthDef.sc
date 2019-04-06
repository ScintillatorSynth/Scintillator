TestScinthDef : UnitTest {

	test_single_vgen_no_inputs {
		var build = XPos.fg;

		// Should have built an XPos object at fragment rate with no inputs.
		this.assertEquals(build.class, XPos);
		this.assertEquals(build.rate, \fragment);
		this.assert(build.inputs.isArray);
		this.assertEquals(build.inputs.size, 0);
	}

	test_single_vgen_constant_inputs {
		var build = VSinOsc.fg(2, pi);

		// Should have built a VSinOsc object at fragment rate with const
		// inputs.
		this.assertEquals(build.class, VSinOsc);
		this.assertEquals(build.rate, \fragment);
		this.assert(build.inputs.isArray);
		this.assertEquals(build.inputs.size, 2);
		this.assert(build.inputs[0].isNumber);
		this.assertEquals(build.inputs[0], 2);
		this.assert(build.inputs[1].isNumber);
		this.assertEquals(build.inputs[1], pi);
	}

	test_simple_chain {
		var build = VSinOsc.fg(freq: YPos.fg, phase: -1.0);

		// Expecting a VSinOsc with a YPos in the first input position.
		this.assertEquals(build.class, VSinOsc);
		this.assertEquals(build.rate, \fragment);
		this.assert(build.inputs.isArray);
		this.assertEquals(build.inputs.size, 2);
		this.assertEquals(build.inputs[0].class, YPos);
		this.assert(build.inputs[0].inputs.isArray);
		this.assertEquals(build.inputs[0].inputs.size, 0);
		this.assert(build.inputs[1].isNumber);
		this.assertEquals(build.inputs[1], -1.0);
	}

	test_unary_vgen_chain {
		var build = ZPos.fg.neg;

		this.assertEquals(build.class, UnaryOpVGen);
		this.assertEquals(build.operator, \neg);
		this.assert(build.inputs.isArray);
		this.assertEquals(build.inputs.size, 1);
		this.assertEquals(build.inputs[0].class, ZPos);
		this.assert(build.inputs[0].inputs.isArray);
		this.assertEquals(build.inputs[0].inputs.size, 0);
	}

	// TODO: test/refine multichannel expansion

}