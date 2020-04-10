BasicOpVGen : VGen {
	// To avoid a proliferation of basic op VGen objects we override the name function
	// to return this.
	var vgenName;

	name {
		^vgenName;
	}
}

UnaryOpVGen : BasicOpVGen {

	*new { |selector, vgen|
		var unaryName;
		switch (selector,
			'neg', { unaryName = 'VNeg' },
			'reciprocal', { unaryName = 'VReciprocal' },
			'bitNot', { ^thisMethod.notYetImplemented },
			'abs', { unaryName = 'VAbs' },
			'asFloat', { ^thisMethod.notYetImplemented },  // everything's a float
			'asInteger', { ^thisMethod.notYetImplemented },
			'ceil', { unaryName = 'VCeil' },
			'floor', { unaryName = 'VFloor' },
			'frac', { unaryName = 'VFract' },
			'sign', { unaryName = 'VSign' },
			'squared', { ^BinaryOpVGen.new('*', vgen, vgen) },
			'cubed', { ^BinaryOpVGen.new('*', BinaryOpVGen.new('*', vgen, vgen), vgen) },
			'sqrt', { unaryName = 'VSqrt' },
			'exp', { unaryName = 'VExp' },
			'midicps', { ^thisMethod.notYetImplemented },
			'cpsmidi', { ^thisMethod.notYetImplemented },
			'midiratio', { ^thisMethod.notYetImplemented },
			'ratiomidi', { ^thisMethod.notYetImplemented },
			'ampdb', { ^thisMethod.notYetImplemented },
			'dbamp', { ^thisMethod.notYetImplemented },
			'octcps', { ^thisMethod.notYetImplemented },
			'cpsoct', { ^thisMethod.notYetImplemented },
			'log', { unaryName = 'VLog' },
			'log2', { unaryName = 'VLog2' },
			'log10', { ^thisMethod.notYetImplemented },
			'sin', { unaryName = 'VSin' },
			'cos', { unaryName = 'VCos' },
			'tan', { unaryName = 'VTan' },
			'asin', { unaryName= 'VASin' },
			'acos', { unaryName = 'VACos' },
			'atan', { unaryName = 'VATan' },
			'sinh', { ^thisMethod.notYetImplemented },
			'cosh', { ^thisMethod.notYetImplemented },
			'tanh', { ^thisMethod.notYetImplemented },
			'rand', { ^thisMethod.notYetImplemented },
			'rand2', { ^thisMethod.notYetImplemented },
			'linrand', { ^thisMethod.notYetImplemented },
			'bilinrand', { ^thisMethod.notYetImplemented },
			'sum3rand', { ^thisMethod.notYetImplemented },
			'distort', { ^thisMethod.notYetImplemented },
			'softclip', { ^thisMethod.notYetImplemented },
			'coin', { ^thisMethod.notYetImplemented },
			'even', { ^thisMethod.notYetImplemented },
			'odd', { ^thisMethod.notYetImplemented },
			'rectWindow', { ^thisMethod.notYetImplemented },
			'hanWindow', { ^thisMethod.notYetImplemented },
			'welWindow', { ^thisMethod.notYetImplemented },
			'triWindow', { ^thisMethod.notYetImplemented },
			'scurve', { ^thisMethod.notYetImplemented },
			'ramp', { ^thisMethod.notYetImplemented },
			'isPositive', { ^thisMethod.notYetImplemented },
			'isNegative', { ^thisMethod.notYetImplemented },
			'isStrictlyPositive', { ^thisMethod.notYetImplemented },
			'rho', { ^thisMethod.notYetImplemented },
			'theta', { ^thisMethod.notYetImplemented },
			'not', { ^thisMethod.notYetImplemented },
			'ref', { ^thisMethod.notYetImplemented },
			{ "Unknown VGen unary operation: %".format(selector).error; ^nil; }
		);
		// Unary operators should take on whatever rate the
		// target VGen is running.
		^this.multiNew(vgen.rate, unaryName, vgen);
	}

	init { |unaryName, vgen|
		vgenName = unaryName;
		rate = vgen.rate;
		inputs = vgen.asArray;
	}

	inputDimensions {
		^[[1], [2], [3], [4]];
	}

	outputDimensions {
		^[[1], [2], [3], [4]];
	}
}

BinaryOpVGen : BasicOpVGen {
	*new { |selector, a, b|
		var binaryName;
		switch (selector,
			'rotate', { ^thisMethod.notYetImplemented },
			'dist', { ^thisMethod.notYetImplemented },
			'+', { binaryName = 'VAdd' },
			'-', { binaryName = 'VSub' },
			'*', { binaryName = 'VMul' },
			'/', { binaryName = 'VDiv' },
			'div', { binaryName = 'VDiv' },  // TODO: is this really the same as /?
			'mod', { binaryName = 'VMod' },
			'pow', { binaryName = 'VPow' },
			'min', { binaryName = 'VMin' },
			'max', { binaryName = 'VMax' },
			'<', { ^thisMethod.notYetImplemented },
			'<=', { ^thisMethod.notYetImplemented },
			'>', { ^thisMethod.notYetImplemented },
			'>=', { ^thisMethod.notYetImplemented },
			'bitAnd', { ^thisMethod.notYetImplemented },
			'bitOr', { ^thisMethod.notYetImplemented },
			'bitXor', { ^thisMethod.notYetImplemented },
			'hammingDistance', { ^thisMethod.notYetImplemented },
			'lcm', { ^thisMethod.notYetImplemented },
			'gcd', { ^thisMethod.notYetImplemented },
			'round', { ^thisMethod.notYetImplemented },
			'roundUp', { ^thisMethod.notYetImplemented },
			'trunc', { ^thisMethod.notYetImplemented },
			'atan2', { binaryName = 'VATan2' },
			'hypot', { ^thisMethod.notYetImplemented },
			'hypotApx', { ^thisMethod.notYetImplemented },
			'leftShift', { ^thisMethod.notYetImplemented },
			'rightShift', { ^thisMethod.notYetImplemented },
			'unsignedRightShift', { ^thisMethod.notYetImplemented },
			'ring1', { ^thisMethod.notYetImplemented },
			'ring2', { ^thisMethod.notYetImplemented },
			'ring3', { ^thisMethod.notYetImplemented },
			'ring4', { ^thisMethod.notYetImplemented },
			'difsqr', { ^thisMethod.notYetImplemented },
			'sumsqr', { ^thisMethod.notYetImplemented },
			'sqrsum', { ^thisMethod.notYetImplemented },
			'sqrdif', { ^thisMethod.notYetImplemented },
			'absdif', { ^thisMethod.notYetImplemented },
			'thresh', { ^thisMethod.notYetImplemented },
			'amclip', { ^thisMethod.notYetImplemented },
			'scaleneg' , { ^thisMethod.notYetImplemented },
			'clip2', { ^thisMethod.notYetImplemented },
			'fold2', { ^thisMethod.notYetImplemented },
			'wrap2', { ^thisMethod.notYetImplemented },
			'excess', { ^thisMethod.notYetImplemented },
			'firstArg', { ^thisMethod.notYetImplemented },
			'rrand', { ^thisMethod.notYetImplemented },
			'exprand', { ^thisMethod.notYetImplemented },
			'@', { ^thisMethod.notYetImplemented }, // TODO: what does this one do?
			'||', { ^thisMethod.notYetImplemented },
			'&&', { ^thisMethod.notYetImplemented },
			'xor', { ^thisMethod.notYetImplemented },
			'nand', { ^thisMethod.notYetImplemented },
			{ "Unknown VGen binary operation: %".format(selector).error; ^nil; }
		);
		^this.multiNew(\fragment, binaryName, a, b);
	}

	init { |binaryName, a, b|
		vgenName = binaryName;
		rate = \fragment;
		inputs = [a, b];
	}

	// Allow scalar products with vectors to produce vectors.
	inputDimensions {
		^[[1, 1], [2, 2], [3, 3], [4, 4]] ++ [[1, 2], [1, 3], [1, 4]] ++ [[2, 1], [3, 1], [4, 1]];
	}

	outputDimensions {
		^[[1], [2], [3], [4]] ++ [[2], [3], [4]] ++ [[2], [3], [4]];
	}

}

