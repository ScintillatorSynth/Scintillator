// Generates a series of images for validation of individual VGens and
// integration testing of the render stack.
(
var testImagesPath, imageOut, options, server, c, d, testDefs, keptScinths;
var bar, colors, positions, scinths, expectedTree, groupSnap, checkTree, groups;
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
	options.logLevel = 2;
	options.createWindow = false;
	options.frameRate = 0;
	// Use prime numbers for image dimensions, to test any image alignment/padding assumptions.
	options.width = 397;
	options.height = 389;
	options.swiftshader = true;
	options.vulkanValidation = true;
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
				ScinImageBuffer.read(server, testImagesPath +/+ img["file"], bufnum: img["number"].asInteger);
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
				"*** Kept Scinths not empty, but test requires that it is.".postln;
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
				var pairs = [];
				// .asPairs not supported in older sclang versions
				params[0].keys.do({ |k| pairs = pairs.add(k); pairs = pairs.add(params[0].at(k).asFloat); });
				scinth = Scinth.new(shortName, args: pairs);
			}, {
				scinth = Scinth.new(shortName);
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
				var pairs = [];
				params[i].keys.do({ |k| pairs = pairs.add(k); pairs = pairs.add(params[i].at(k).asFloat); });
				scinth.set(*pairs);
			});
			dt = dt.asInteger;
			fileName = imageOut +/+ def["category"] +/+ def["shortName"] ++ "_" ++ t.asString ++ ".png";
			"queing screenShot at %".format(fileName).postln;
			if (server.queueScreenShotSync(fileName, "image/png",
				{ |r| d.test = true; d.signal; }, c).not, {
				"failed to queue screenshot for % at t=%".format(def["shortName"], t).postln;
				"*** SCRIPT FAILED ***".postln;
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

	// Group and node ordering test.
	server.defaultGroup.freeAll;
	(imageOut +/+ "groups").mkdir;

	bar = ScinthDef.new(\bar, { |x = 0.0, r = 1.0, g = 1.0, b = 1.0|
		var xpos = (VX.pr(VNormPos.pr) - x).abs;
		VRGBAOut.pr(r, g, b, 1.0 - VStep.pr(0.125, xpos));
	}).add;
	server.sync;
	colors = [
		[ 1.0,   0.224, 0.0   ],
		[ 1.0,   0.498, 0.353 ],
		[ 1.0,   0.357, 0.173 ],
		[ 0.855, 0.192, 0.0   ],
		[ 0.671, 0.149, 0.0   ],
		[ 0.031, 0.486, 0.729 ],
		[ 0.302, 0.616, 0.784 ],
		[ 0.161, 0.541, 0.745 ],
		[ 0.02,  0.361, 0.545 ],
		[ 0.016, 0.282, 0.424 ],
		[ 0.482, 0.929, 0.0   ],
		[ 0.651, 0.941, 0.333 ],
		[ 0.561, 0.933, 0.161 ],
		[ 0.404, 0.773, 0.0   ],
		[ 0.314, 0.604, 0.0   ]
	];
	positions = [];
	15.do({ |i|
		var x = (i - 7) / 7.0;
		positions = positions.add(x);
	});
	groupSnap = { |name|
		d.test = false;
		if (server.queueScreenShotSync(imageOut +/+ "groups" +/+ name, "image/png",
			{ |r| d.test = true; d.signal; }, c).not, {
			"failed to queue screenshot for groups/%".format(name).postln;
			"*** SCRIPT FAILED ***".postln;
			-1.exit;
		});
		server.advanceFrame(1, 1);
		d.wait;
	};
	checkTree = { |group|
		d.test = false;
		group.queryTree({ |msg|
			msg.do({ |ele, i|
				// Floats won't match exactly due to truncation to 32 bits for seding via
				// OSC, so we check that the truncated values match.
				if (ele.class == 0.224.class, {
					if (ele.as32Bits != expectedTree[i].as32Bits, {
						"query float mismatch at position %, got %, expected %".format(
							i, ele, expectedTree[i]).postln;
						"*** SCRIPT FAILED ***".postln;
					});
				}, {
					if (ele != expectedTree[i], {
						"query differs at position %, got %, expected %".format(
							i, ele, expectedTree[i]).postln;
						"*** SCRIPT FAILED ***".postln;
						-1.exit;
					});
				});
			});
			d.test = true;
			d.signal;
		}, true);
		d.wait;
	};

	// left-to-right with default node ordering
	scinths = [];
	expectedTree = ['/scin_g_queryTree.reply', 1, server.defaultGroup.nodeID, 15];
	15.do({ |i|
		scinths = scinths.add(Scinth.new(\bar, [x: positions[i],
		r: colors[i][0], g: colors[i][1], b: colors[i][2]]));
		expectedTree = expectedTree ++
		[scinths[i].nodeID, -1, \bar, 4, \x, positions[i], \r, colors[i][0],
			\g, colors[i][1], \b, colors[i][2]];
	});

	groupSnap.value("left-to-right.png");
	checkTree.value(server.defaultGroup);
	server.defaultGroup.dumpTree;

	// outside-in with groups moveToHead/moveToTail
	groups = [];
	expectedTree = ['/scin_g_queryTree.reply', 1, server.defaultGroup.nodeID, 8];

	8.do({ |i| groups = groups.add(ScinGroup.new) });
	7.do({ |i|
		groups[i].moveNodeToHead(scinths[i]);
		groups[i].moveNodeToTail(scinths[14 - i]);
		expectedTree = expectedTree ++ [groups[i].nodeID, 2];
	});
	scinths[7].moveToHead(groups[7]);
	expectedTree = expectedTree ++ [groups[7].nodeID, 1];

	groupSnap.value("outside-in.png");
	checkTree.value(server.defaultGroup);
	server.defaultGroup.dumpTree;

	// inside-out testing moveBefore and moveAfter
	scinths[14].moveAfter(scinths[7]);
	scinths[7].moveBefore(scinths[0]);
	scinths[0].moveBefore(scinths[14]);

	scinths[1].moveAfter(scinths[6]);
	scinths[6].moveBefore(scinths[13]);
	scinths[13].moveAfter(scinths[1]);
	scinths[8].moveBefore(scinths[6]);

	scinths[2].moveAfter(scinths[5]);
	scinths[5].moveBefore(scinths[12]);
	scinths[12].moveAfter(scinths[2]);
	scinths[9].moveBefore(scinths[5]);

	scinths[3].moveAfter(scinths[4]);
	scinths[4].moveBefore(scinths[11]);
	scinths[11].moveAfter(scinths[3]);
	scinths[10].moveBefore(scinths[4]);

	// Swap sizes of the first and last subgroup, group order should stay the same.
	expectedTree[5] = 1;
	expectedTree[19] = 2;

	groupSnap.value("inside-out.png");
	checkTree.value(server.defaultGroup);
	server.defaultGroup.dumpTree;

	// right-to-left with nested groups
	7.do({ |i|
		scinths[14 - (2 * i)].moveToTail(groups[i]);
		scinths[13 - (2 * i)].moveToHead(groups[i]);
	});
	scinths[0].moveToTail(groups[7]);
	groups[1].moveNodeToHead(groups[0]);
	groups[2].moveNodeToHead(groups[1]);
	groups[3].moveNodeToHead(groups[2]);
	groups[3].moveNodeToTail(groups[4]);
	groups[4].moveNodeToTail(groups[5]);
	groups[5].moveNodeToTail(groups[6]);
	groups[6].moveNodeToTail(groups[7]);

	groupSnap.value("right-to-left.png");
	server.defaultGroup.dumpTree;

    server.quit;
    1.wait;
    "*** SCRIPT OK ***".postln;
    0.exit;
}
)

