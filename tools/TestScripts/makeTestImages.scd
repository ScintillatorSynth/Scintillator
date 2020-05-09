// Generates a series of images for validation of individual VGens and
// integration testing of the render stack.
(
var testImagesPath, imageOut, options, server, c, d, testDefs, keptScinths;
if (thisProcess.argv.size < 1, {
    "requires at least one argument, the path to the test images repository.".postln;
    "*** SCRIPT FAILED ***".postln;
    -1.exit;
});
// Remaining arguments, if any, should be environment variables to set before invoking the server.
thisProcess.argv[1..].do({ |pairs|
    var splitPairs = pairs.split($=);
    "setting environment variable % to %".format(splitPairs[0], splitPairs[1]).postln;
    splitPairs[0].setenv(splitPairs[1]);
});
fork {
	testImagesPath = thisProcess.argv[0] +/+ "sourceMedia";
	c = Condition.new;
	d = Condition.new;
    imageOut = Scintillator.path +/+ "build" +/+ "testing";
	testDefs = (Scintillator.path +/+ "tools" +/+ "TestScripts" +/+ "testManifest.yaml").parseYAMLFile;
	options = ScinServerOptions.new;
	options.logLevel = 0;
	options.createWindow = false;
	options.frameRate = 0;
	options.width = 400;
	options.height = 400;
	options.swiftshader = true;
	if (thisProcess.platform.name !== 'windows', {
		options.vulkanValidation = true;
	});
	options.onServerError = { |exitCode|
		"*** nonzero exit code % on server, exiting.".format(exitCode).postln;
        "*** SCRIPT FAILED ***".postln;
		exitCode.exit;
	};
	server = ScinServer.new(options);
	server.bootSync(c);
	keptScinths = IdentityDictionary.new;

	testDefs.do({ |def|
		var scinth, fileName, params, shortName, keep, t;
		// Make sure output subdirectory exists.
		(imageOut +/+ def["category"]).mkdir;

		// Load any images first.
		if (def["images"].notNil, {
			def["images"].do({ |img|
				ImageBuffer.read(server, testImagesPath +/+ img["file"], bufnum: img["number"].asInteger);
			});
			server.sync;
		});

		// Check for parameters, in case there are initial values to set.
		if (def["parameters"].notNil, {
			params = def["parameters"];
		}, {
			params = ["none"].extend(def["captureTimes"].size, "none");
		});

		// If there are Scinths that we've kept that we are now to free, free them.
		if (def["free"].notNil, {
			def["free"].do({ |name|
				var freeName = name.asSymbol;
				var freeScinth = keptScinths.at(freeName);
				"freeing Scinth %".format(name).postln;
				freeScinth.free;
				keptScinths.removeAt(freeName);
			});
		});

		// We have a check in place to make sure that all kept scinths have been freed, so check if that is requested
		// and, if so, then assert if it is not the case.
		if (def["assertAllFree"].notNil, {
			if (keptScinths.isEmpty.not, {
				"*** Kepth Scinths not empty, but test requires that it is.".postln;
				"*** SCRIPT FAILED ***".postln;
				1.exit;
			});
		});

		// If there is a ScinthDef to send to the server send it.
		if (def["scinthDef"].size > 0, {
			shortName = def["shortName"].asSymbol;
			"sending ScinthDef for %".format(shortName).postln;
			ScinthDef.new(shortName, def["scinthDef"].interpret).add;
			server.sync;
			if (params[0].class == Dictionary, {
				var pairs = params[0].asPairs.collect({ |item, j| if ((j % 2) == 1, { item.asFloat; }, { item }) });
				scinth = Scinth.new(shortName, args: pairs);
			}, {
				scinth = Scinth.new(shortName, 1);
			});
		});
		keep = false;
		if (def["keep"].notNil, {
			keep = def["keep"] == "true";
		});

		// Now advance time and record images.
		t = 0;
		def["captureTimes"].do({ |dt, i|
			d.test = false;
			if (i > 0 and: { params[i].class == Dictionary }, {
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
			if (keep, {
				"kept Scinth %".format(shortName).postln;
				keptScinths.put(shortName, scinth);
			}, {
				scinth.free;
			});
		});
	});

    server.quit;
    1.wait;
    "*** SCRIPT OK ***".postln;
    0.exit;
}
)

