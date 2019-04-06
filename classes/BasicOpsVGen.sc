BasicOpVGen : VGen {
	var <operator;
}

UnaryOpVGen : BasicOpVGen {

	*new { | selector, vGen |
		// Unary operators should take on whatever rate the
		// target VGen is running.
		^this.multiNew(vGen.rate, selector, vGen);
	}

	init { | theOperator, theInput |
		operator = theOperator;
		rate = theInput.rate;
		inputs = theInput.asArray;
	}
}


VMulAdd : VGen {
	*new { | in, mul = 1.0, add = 0.0 |
		var args = [in, mul, add].asVGenInput(this);
		var rate = args.rate;
		^this.multiNewList([rate] ++ args);
	}

	*singleNew { | rate, in, mul, add |
		var minus, nomul, noadd;

		// Just as in MulAdd, remove degenerate cases.
		// Multiply by zero means this reduces to the add value.
		if (mul == 0.0, { ^add });
		minus = mul == -1.0;
		nomul = mul == 1.0;
		noadd = add == 0.0;
		// If mul and add are at defaults we can remove this node.
		if (nomul && noadd, { ^in });
		// If only negating, just negate.
		if (minus && noadd, { ^in.neg });
		// If only multiplying, just multiply.
		if (noadd, { ^in * mul });
		// If negating only with mul, just subtract input.
		if (minus, { ^add - in });
		// If only adding, just add.
		if (nomul, { ^in + add });

		// Some VGens can also serve as MulAdds, as the
		// math includes these operations. If so, replace
		// the explicit node with an intrinsic one.
		// TODO

		^((in * mul) + add);
	}

	init { | in, mul, add |
		inputs = [in, mul, add];  // huh?
		rate = inputs.rate;
	}
}