AbstractVideoOut : VGen {

}

VOut : AbstractVideoOut {
	*fg { | bus, channelsArray |
		this.multiNewList(['fragment', bus] ++ channelsArray.asArray);
		^0.0;
	}
}
