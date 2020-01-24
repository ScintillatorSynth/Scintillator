// Generates a series of images for validation of individual VGens and
// integration testing of the render stack.
(
var quarkPath, imageOut, options, server, c, d, queueScreen, scinthDef, scinth;
if (thisProcess.argv.size != 1, {
    "requires one argument, the path to the Scintillator Quark directory.".postln;
    -1.exit;
});
fork {
	// OSCFunc.trace(true);
	quarkPath = thisProcess.argv[0];
    imageOut = quarkPath +/+ "build" +/+ "testing";
	options = ScinServerOptions.new;
	options.quarkPath = quarkPath;
	options.logLevel = 2;
	options.createWindow = false;
	options.frameRate = 0;
	options.swiftshader = true;

	server = ScinServer.new(options);
	c = Condition.new;
	d = Condition.new;
	queueScreen = { |fileName|
		d.test = false;
		if (server.queueScreenShotSync(imageOut +/+ fileName, "image/png",
			{ |r| d.test = true; d.signal; }, c).not, {
			"failed to queue screenshot %".format(fileName).postln;
			-1.exit;
		});
	};

	server.bootSync(c);
	// server.dumpOSC(true);

	queueScreen.value("firstFrame.png");
	server.advanceFrame(10, 1);
	d.wait;

	scinthDef = ScinthDef.new(\scinOsc, {
		RGBOut.fg(
			ScinOsc.fg(Length.fg(NormPos.fg)),
			ScinOsc.fg(2, VX.fg(NormPos.fg)),
            ScinOsc.fg(3, VY.fg(NormPos.fg)));
    }).add;
	server.sync;
	scinth = Scinth.new(\scinOsc, 1);

	queueScreen.value("scinOsc_a.png", "image/png");
	server.advanceFrame(10, 1);
	d.wait;

	queueScreen.value("scinOsc_b.png", "image/png");
	server.advanceFrame(10, 1);
	d.wait;

	queueScreen.value("scinOsc_c.png", "image/png");
	server.advanceFrame(10, 1);
	d.wait;

    server.quit;
    1.wait;
    0.exit;
}
)
