AbstractVideoOut : VGen {

}

VOut : AbstractVideoOut {
	*fr { | bus, channelsArray |
		this.multiNewList(['fragment', bus] ++ channelsArray.asArray);
		^0.0;
	}
}
