ScinShape {
	isScinShape { ^true }

	asYAML {
		Error.new("Base ScinShape object not suitable for shape specification. Did you mean to use ScinQuad?").throw;
	}
}

ScinQuad : ScinShape {
	var <>widthEdges;
	var <>heightEdges;

	*new { |widthEdges = 1, heightEdges = 1|
		^super.newCopyArgs(widthEdges, heightEdges);
	}

	asYAML { |depthIndent = ""|
		^"%name: Quad\n%widthEdges: %\n%heightEdges: %\n".format(
			depthIndent, depthIndent, widthEdges, depthIndent, heightEdges);
	}
}

+ Object {
	isScinShape { ^false }
}