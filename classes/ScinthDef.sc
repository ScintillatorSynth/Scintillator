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

	// VGens call these.
	addVGen { | vGen |
		vGen.scynthIndex = children.size;
		children = children.add(vGen);
	}
}