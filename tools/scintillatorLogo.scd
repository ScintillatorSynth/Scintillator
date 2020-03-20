(
~k = ScinthDef.new(\scinLogo, { |freq = 35.0|
	var pos = NormPos.fg;
	var x = VX.fg(pos);
	var y = VY.fg(pos);
	var dim = x.abs.max(y.abs);
	var vDim = Vec2.fg(dim, dim);
	var square = Step.fg((Length.fg(vDim * freq).sin / 2) + 0.5, 0.5);
	var rotx = (x - y) / 2.sqrt;
	var roty = (x + y) / 2.sqrt;
	var rotDim = rotx.abs.max(roty.abs);
	var rotVdim = Vec2.fg(rotDim, rotDim);
	var rotSquare = Step.fg((Length.fg(rotVdim * (freq * 2.sqrt)).sin / 2) + 0.5, 0.5);
	RGBOut.fg(square, rotSquare, 1.0 - ((Length.fg(pos) / 2).min(1.0)));
}).add;
)