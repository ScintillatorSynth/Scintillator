TestVGen : UnitTest {
	// TODO: rename these test cases to indicate that they are all built with single-output VGens only.

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

		// Should have built a VSinOsc object at fragment rate with const inputs.
		this.assertEquals(build.class, VSinOsc);
		this.assertEquals(build.rate, \fragment);
		this.assert(build.inputs.isArray);
		this.assertEquals(build.inputs.size, 2);
		this.assert(build.inputs[0].isNumber);
		this.assertEquals(build.inputs[0], 2);
		this.assert(build.inputs[1].isNumber);
		this.assertEquals(build.inputs[1], pi);
	}

	test_input_chain_linear {
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

		// Linear chain with UnaryOpVGen with \neg operator.
		this.assertEquals(build.class, UnaryOpVGen);
		this.assertEquals(build.operator, \neg);
		this.assert(build.inputs.isArray);
		this.assertEquals(build.inputs.size, 1);
		this.assertEquals(build.inputs[0].class, ZPos);
		this.assert(build.inputs[0].inputs.isArray);
		this.assertEquals(build.inputs[0].inputs.size, 0);
	}

	test_output_chain_mul_add {
		var build = VSinOsc.fg(freq: 10.0, phase: -pi, mul: 5.0, add: 1.0);

		// Expecting a chain with VMulAdd added on end with appropriate args.
		this.assertEquals(build.class, VMulAdd);
		this.assert(build.inputs.isArray);
		this.assertEquals(build.inputs.size, 3);
		this.assertEquals(build.inputs[0].class, VSinOsc);
		this.assert(build.inputs[0].inputs.isArray);
		this.assertEquals(build.inputs[0].inputs.size, 2);
		this.assertEquals(build.inputs[0].inputs[0], 10.0);
		this.assertEquals(build.inputs[0].inputs[1], -pi);
		this.assert(build.inputs[1].isNumber);
		this.assertEquals(build.inputs[1], 5.0);
		this.assert(build.inputs[2].isNumber);
		this.assertEquals(build.inputs[2], 1.0);
	}
}
