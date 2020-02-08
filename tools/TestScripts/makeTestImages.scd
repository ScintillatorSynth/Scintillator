// Generates a series of images for validation of individual VGens and
// integration testing of the render stack.
(
var quarkPath, imageOut, options, server, c, d, testDefs;
if (thisProcess.argv.size < 1, {
    "requires at least one argument, the path to the Scintillator Quark directory.".postln;
    -1.exit;
});
// Remaining arguments, if any, should be environment variables to set before invoking the server.
thisProcess.argv[1..].do({ |pairs|
    var splitPairs = pairs.split($=);
    "setting environment variable % to %".format(splitPairs[0], splitPairs[1]).postln;
    splitPairs[0].setenv(splitPairs[1]);
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
	options.width = 400;
	options.height = 400;
	options.swiftshader = true;
	options.onServerError = { |exitCode|
		"*** nonzero exit code % on server, exiting.".format(exitCode).postln;
		exitCode.exit;
	};
	server = ScinServer.new(options);
	server.bootSync(c);

	testDefs.do({ |def|
		var scinth, fileName, params, t;
		// Make sure output subdirectory exists.
		(imageOut +/+ def["category"]).mkdir;
		if (def["scinthDef"].size > 0, {
			"sending ScinthDef for %".format(def["shortName"]).postln;
			ScinthDef.new(\test, def["scinthDef"].interpret).add;
			server.sync;
			scinth = Scinth.new(\test, 1);
		});
		if (def["parameters"].notNil, {
			params = def["parameters"];
		}, {
			params = ["none"].extend(def["captureTimes"].size, "none");
		});
		// *** Need to add support for the new parameters: key right here.
		t = 0;
		def["captureTimes"].do({ |dt, i|
			d.test = false;
			if (params[i].class == Dictionary, {
				var pairs = params[i].asPairs.collect({ |item, j| if ((j % 2) == 1, { item.asFloat; }, { item }) });
				scinth.set(*pairs);
			});
			dt = dt.asInteger;
			fileName = imageOut +/+ def["category"] +/+ def["shortName"] ++ "_" ++ t.asString ++ ".png";
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
