// Lower-level script for testing individual OSC commands to Scintillator Server. Bypasses the higher-level classes
// in sclang and sends messages directly to the server.
// TODO: add a timeout on the Condition waits so that missing messages don't hang this script.
(
var versionMajor, versionMinor, versionPatch, gitBranch, gitHash, options, binPath;
var scriptFailed = "*** SCRIPT FAILED ***";
var scriptOk = "*** SCRIPT OK ***";
if (thisProcess.argv.size < 5, {
	"requires at least five arguments:".postln;
	"  Scintillator major, minor, and patch version numbers".postln;
	"  Git branch name and first seven hex digits of the commit hash".postln;
	scriptFailed.postln;
	-1.exit;
});
// Remaining arguments, if any, should be environment variables to set before invoking the server.
thisProcess.argv[5..].do({ |pairs|
    var splitPairs = pairs.split($=);
    "setting environment variable % to %".format(splitPairs[0], splitPairs[1]).postln;
    splitPairs[0].setenv(splitPairs[1]);
});
OSCFunc.trace(true);
versionMajor = thisProcess.argv[0].asInteger;
versionMinor = thisProcess.argv[1].asInteger;
versionPatch = thisProcess.argv[2].asInteger;
gitBranch = thisProcess.argv[3].asSymbol;
gitHash = thisProcess.argv[4].asSymbol;
options = ScinServerOptions.new;
options.logLevel = 2;
options.createWindow = false;
options.frameRate = 0;
options.width = 200;
options.height = 200;
options.dumpOSC = true;
options.swiftshader = true;
options.vulkanValidation = true;
binPath = ScinServerOptions.quarkPath +/+ "bin/scinsynth-x86_64.AppImage";
fork {
	var commandLine, scinPid, udp, c, d, valid, response;
	var gotFrameRate, numWarn, numError, gotWarn, gotError;
	var awakeCount, awakeFunc;
	var testDef, defCount, callbackFunc;
	var testDefPath;
	var nodeCount;
	commandLine = binPath + options.asOptionsString();
	"*** booting scinsynth with command line: %".format(commandLine).postln;
	scinPid = commandLine.unixCmd({ |exitCode, exitPid|
		if (valid, {
			// We'll exit this script with the same exit code as scinsynth provides here.
			"got scinsynth exit code %, terminating.".format(exitCode).postln;
			if (exitCode == 0, {
				scriptOk.postln;
			}, {
				scriptFailed.postln;
			});
			exitCode.exit;
		}, {
			"** didn't get response from /scin_quit".postln;
			scriptFailed.postln;
			1.exit;
		});
	});

	// We'll poll until we get a response.
	udp = NetAddr.new("127.0.0.1", options.portNumber);
	c = Condition.new;
	d = Condition.new;
	c.test = false;
	fork {
		var booted = false;
		var response = OSCFunc.new({ |msg|
			response.free;
			booted = true;
			c.test = true;
			c.signal;
		}, '/scin_status.reply');
		30.do({
			if (booted.not, {
				udp.sendMsg('/scin_status');
				1.wait;
			});
		});
		if (booted.not, {
			"timed out waiting for boot!".postln;
			Platform.killAll("scinsynth");
			scriptFailed.postln;
			1.exit;
		});
	};

	c.wait;

	"*** Testing /scin_status".postln;
	c.test = false;
	valid = false;
	response = OSCFunc.new({ |msg|
		numWarn = msg[4];
		numError = msg[5];
		gotFrameRate = msg[8];
		if (gotFrameRate == options.frameRate, {
			valid = true;
		});
		c.test = true;
		c.signal;
	}, '/scin_status.reply').oneShot;
	udp.sendMsg('/scin_status');
	c.wait;
	if (valid.not, {
		"*** /scin_status test checking framerate failed!".postln;
		"expected % as frame rate, got %".format(
			options.frameRate, gotFrameRate).postln;
		scriptFailed.postln;
		1.exit;
	});

	udp.sendMsg('/scin_logAppend', 3, "Test Warning - ignore");
	numWarn = numWarn + 1;
	udp.sendMsg('/scin_logAppend', 4, "Test Error - ignore");
	numError = numError + 1;

	c.test = false;
	valid = false;
	response = OSCFunc.new({ |msg|
		defCount = msg[3];
		gotWarn = msg[4];
		gotError = msg[5];
		if (numWarn == gotWarn and: { numError == gotError }, {
			valid = true;
		});
		c.test = true;
		c.signal;
	}, '/scin_status.reply').oneShot;
	udp.sendMsg('/scin_status');
	c.wait;
	if (valid.not, {
		"*** /scin_status test on error warning counts failed!".postln;
		"expected % warnings and % errors, got % warnings and % errors".format(
			numWarn, numError, gotWarn, gotError).postln;
		scriptFailed.postln;
		1.exit;
	});


	"*** Testing /scin_sync".postln;
	// First test with empty work queue.
	c.test = false;
	valid = false;
	response = OSCFunc.new({ |msg|
		if (msg[1] == 23, {
			valid = true;
		});
		c.test = true;
		c.signal;
	}, '/scin_synced').oneShot;
	udp.sendMsg('/scin_sync', 23);
	c.wait;
	if (valid.not, {
		"*** /scin_sync test on empty work queue failed!".postln;
		"didn't get back same ID as what was sent.".postln;
		scriptFailed.postln;
		1.exit;
	});

	// Now put a few workers to sleep and wait for sync again.
	c.test = false;
	d.test = false;
	valid = false;
	response = OSCFunc.new({ |msg|
		if (msg[1] == 19, {
			valid = true;
		});
		c.test = true;
		c.signal;
	}, '/scin_synced').oneShot;
	awakeCount = 0;
	awakeFunc = OSCFunc.new({ |msg|
		if (awakeCount == 0, {
			d.test = true;
			d.signal;
		});
		awakeCount = awakeCount + 1;
		if (awakeCount == 10, {
			d.test = true;
			d.signal;
		});
	}, '/scin_awake');
	10.do({
		udp.sendMsg('/scin_sleepFor', 1);
	});
	// Wait until we've gotten at least one of the awakes back, to break
	// the race of the scin_sync being processed before any of sleeps are
	// enqueued.
	d.wait;
	d.test = false;
	udp.sendMsg('/scin_sync', 19);
	c.wait;
	d.wait;
	if (valid.not or: { awakeCount != 10 }, {
		"*** /scin_sync test on busy queue failed!".postln;
		"expected 5 dones, got back %".format(awakeCount).postln;
		scriptFailed.postln;
		1.exit;
	});
	awakeFunc.free;

	// No easy way to test /scin_logLevel from sclang, could fire up another scinsynth process
	// synchronously and look at captured stdout strings.

	"*** Testing /scin_version".postln;
	c.test = false;
	valid = false;
	response = OSCFunc.new({ |msg|
		valid = true;
		if (msg[1] != 'scinsynth', {
			valid = false;
		});
		if (msg[2] != versionMajor, {
			valid = false;
		});
		if (msg[3] != versionMinor, {
			valid = false;
		});
		if (msg[4] != versionPatch, {
			valid = false;
		});
		if (msg[5] != gitBranch, {
			"/scin_version branch differs, expected: %, got: %".format(gitBranch, msg[5]).warn;
		});
		if (msg[6] != gitHash, {
			"/scin_version hash differs, expected: %, got: %".format(gitHash, msg[6]).warn;
		});
		c.test = true;
		c.signal;
	}, '/scin_version.reply').oneShot;
	udp.sendMsg('/scin_version');
	c.wait;
	if (valid.not, {
		"*** /scin_version test got back different results than expected.".postln;
		"expected version major: % minor: % patch: % branch: % hash: %".format(
			versionMajor, versionMinor, versionPatch, gitBranch, gitHash).postln;
		scriptFailed.postln;
		1.exit;
	});

	"*** Testing /scin_d_recv".postln;
	// Get def count.
	c.test = false;
	response = OSCFunc.new({ |msg|
		defCount = msg[3];
		c.test = true;
		c.signal;
	}, '/scin_status.reply').oneShot;
	udp.sendMsg('/scin_status');
	c.wait;

	// Empty/absent def should still respond.
	c.test = false;
	valid = false;
	response = OSCFunc.new({ |msg|
		c.test = true;
		c.signal;
	}, '/scin_done').oneShot;
	udp.sendMsg('/scin_d_recv');
	c.wait;

	// Expecting no increase to def count.
	c.test = false;
	valid = false;
	response = OSCFunc.new({ | msg|
		if (defCount == msg[3], {
			valid = true;
		});
		c.test = true;
		c.signal;
	}, '/scin_status.reply');
	udp.sendMsg('/scin_status');
	c.wait;

	if (valid.not, {
		"*** /scin_d_recv added to valid def count but no valid def sent.".postln;
		scriptFailed.postln;
		1.exit;
	});

	c.test = false;
	d.test = false;
	valid = false;
	testDef = ScinthDef.new(\t, { BWOut.fr(1.0); });
	response = OSCFunc.new({ |msg|
		c.test = true;
		c.signal;
	}, '/scin_done').oneShot;
	callbackFunc = OSCFunc.new({ |msg|
		if (defCount == msg[3], {
			valid = true;
		});
		d.test = true;
		d.signal;
	}, '/scin_status.reply').oneShot;
	defCount = defCount + 1;
	udp.sendMsg('/scin_d_recv', testDef.asYAML, ['/scin_status']);
	c.wait;
	d.wait;
	if (valid.not, {
		"*** /scin_d_recv status def count not received or incorrect.".postln;
		scriptFailed.postln;
		1.exit;
	});

	"*** Testing /scin_d_load".postln;
	// Empty/absent file path should still respond
	c.test = false;
	response = OSCFunc.new({ |msg|
		c.test = true;
		c.signal;
	}, '/scin_done');
	udp.sendMsg('/scin_d_load');
	c.wait;

	// Empty YAML file.
	c.test = false;
	d.test = false;
	valid = false;
	testDefPath = ScinServerOptions.quarkPath +/+ "tools" +/+ "TestScripts" +/+ "testDefs";
	callbackFunc = OSCFunc.new({ |msg|
		if (msg[1] == 'xx', {
			valid = true;
		});
		d.test = true;
		d.signal;
	}, '/bounceBack').oneShot;
	udp.sendMsg('/scin_d_load', testDefPath +/+ "emptyYAML.yaml",  ['/scin_echo', ['/bounceBack', "xx"].asRawOSC]);
	c.wait;
	d.wait;
	if (valid.not, {
		"*** /scin_d_load echo back not received.".postln;
		scriptFailed.postln;
		1.exit;
	});

	// Valid YAML file.
	c.test = false;
	d.test = false;
	valid = false;
	callbackFunc = OSCFunc.new({ |msg|
		if (defCount == msg[3], {
			valid = true;
		});
		d.test = true;
		d.signal;
	}, '/scin_status.reply').oneShot;
	defCount = defCount + 2;
	udp.sendMsg('/scin_d_load', testDefPath +/+ "t1-2.yaml", ['/scin_status']);
	c.wait;
	d.wait;
	response.free;
	if (valid.not, {
		"*** /scin_d_load got incorrect def count back.".postln;
		scriptFailed.postln;
		1.exit;
	});

	"*** Testing /scin_d_loadDir".postln;
	// Bad path should result in no increase to def count.
	c.test = false;
	valid = false;
	response = OSCFunc.new({ |msg|
		c.test = true;
		c.signal;
	}, '/scin_done');
	udp.sendMsg('/scin_d_loadDir', "");
	c.wait;
	callbackFunc = OSCFunc.new({ |msg|
		if (defCount == msg[3], {
			valid = true;
		});
		d.test = true;
		d.signal;
	}, '/scin_status.reply');
	d.test = false;
	udp.sendMsg('/scin_status');
	d.wait;

	if (valid.not, {
		"*** calling /scin_d_loadDir with invalid path caused change in def count.".postln;
		scriptFailed.postln;
		1.exit;
	});

	// Loading testDefPath should add one new def (t3) to the defs.
	c.test = false;
	d.test = false;
	valid = false;
	defCount = defCount + 1;
	udp.sendMsg('/scin_d_loadDir', testDefPath, ['/scin_status']);
	c.wait;
	d.wait;
	if (valid.not, {
		"*** calling /scin_d_loadDir on % resulted in incorrect def count".format(testDefPath).postln;
		scriptFailed.postln;
		1.exit;
	});
	response.free;
	callbackFunc.free;

	"*** Testing /scin_d_free".postln;
	// NOTE: Assumes t1, t2, and t3 have all been defined in previous tests.
	valid = false;
	c.test = false;
	response = OSCFunc.new({ |msg|
		nodeCount = msg[1];
		if (defCount == msg[3], {
			valid = true;
		});
		c.test = true;
		c.signal;
	}, '/scin_status.reply').oneShot;
	defCount = defCount - 2;
	udp.sendMsg('/scin_d_free', "t1", "t3");
	udp.sendMsg('/scin_status');
	c.wait;
	if (valid.not, {
		"*** calling /scin_d_free resulted in incorrect def count".postln;
		scriptFailed.postln;
		1.exit;
	});

	// Important to test freeing a ScinthDef that has an active Scinth rendering.
	response = OSCFunc.new({ |msg|
		c.test = true;
		c.signal;
	}, '/scin_done');
	udp.sendMsg('/scin_s_new', "t2", 1);
	udp.sendMsg('/scin_s_new', "t2", 2);
	udp.sendMsg('/scin_s_new', "t2", 3);
	c.test = false;
	udp.sendMsg('/scin_nrt_advanceFrame', 1, 1);
	c.wait;
	defCount = defCount - 1;
	udp.sendMsg('/scin_d_free', "t2", "t3");
	c.test = false;
	valid = false;
	udp.sendMsg('/scin_status');
	c.wait;
	if (valid.not, {
		"*** calling /scin_d_free with already freed ScinthDef resulted in incorrect def count.".postln;
		scriptFailed.postln;
		1.exit;
	});
	udp.sendMsg('/scin_n_run', 1, 0);
	c.test = false;
	udp.sendMsg('/scin_nrt_advanceFrame', 1, 1);
	c.wait;

	udp.sendMsg('/scin_n_free', 1);
	c.test = false;
	udp.sendMsg('/scin_nrt_advanceFrame', 1, 1);
	c.wait;

	udp.sendMsg('/scin_n_free', 2);
	c.test = false;
	udp.sendMsg('/scin_nrt_advanceFrame', 1, 1);
	c.wait;

	udp.sendMsg('/scin_n_free', 3);
	c.test = false;
	udp.sendMsg('/scin_nrt_advanceFrame', 1, 1);
	c.wait;

	response.free;

	// We test quit last, should terminate the server but first send a final /scin_done message.
	"*** Testing /scin_quit".postln;
	valid = false;
	response = OSCFunc.new({ |msg|
		response.free;
		valid = true;
	}, '/scin_done');
	udp.sendMsg('/scin_quit');
}
)
