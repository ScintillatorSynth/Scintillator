// Generates a series of images for validation of individual VGens and
// integration testing of the render stack.
(
var quarkPath, imageOut, options, server, c, d, testDefs;
if (thisProcess.argv.size != 1, {
    "requires one argument, the path to the Scintillator Quark directory.".postln;
    -1.exit;
});
fork {
	quarkPath = thisProcess.argv[0];
	c = Condition.new;
	d = Condition.new;
    imageOut = quarkPath +/+ "build" +/+ "testing";
	testDefs = (quarkPath +/+ "tools" +/+ "TestScripts" +/+ "testManifest.yaml").parseYAMLFile;
	options = ScinServerOptions.new;
	options.quarkPath = quarkPath;
	options.logLevel = 2;
	options.createWindow = false;
	options.frameRate = 0;
//	options.swiftshader = true;
	server = ScinServer.new(options);
	server.bootSync(c);

	testDefs.do({ |def|
		var scinth, fileName, t;
		if (def["scinthDef"].size > 0, {
			"sending ScinthDef for %".format(def["shortName"]).postln;
			ScinthDef.new(\test, def["scinthDef"].interpret).add;
			server.sync;
			scinth = Scinth.new(\test, 1);
		});
		t = 0;
		def["captureTimes"].do({ |dt|
			d.test = false;
			dt = dt.asInteger;
			fileName = imageOut +/+ def["shortName"] ++ "_" ++ t.asString ++ ".png";
			"queing screenShot at %".format(fileName).postln;
			if (server.queueScreenShotSync(fileName, "image/png",
				{ |r| d.test = true; d.signal; }, c).not, {
				"failed to queue screenshot for % at t=%".format(def["shortName"], t).postln;
				-1.exit;
			});
			"advancing frame by %".format(dt).postln;
			server.advanceFrame(dt, 1);
			d.wait;
			t = t + dt;
		});
		if (scinth.notNil, {
			scinth.free;
		});
	});

    server.quit;
    1.wait;
    0.exit;
}
)
