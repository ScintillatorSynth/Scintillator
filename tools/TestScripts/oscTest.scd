// Lower-level script for testing individual OSC commands to Scintillator Server. Bypasses the higher-level classes
// in sclang and sends messages directly to the server.
(
var quarkPath, versionMajor, versionMinor, versionPatch, gitBranch, gitHash, options, binPath;
if (thisProcess.argv.size < 6, {
	"requires at least six arguments:".postln;
	"  the path to the Scintillator Quark directory".postln;
	"  Scintillator major, minor, and patch version numbers".postln;
	"  Git branch name and first seven hex digits of the commit hash".postln;
    -1.exit;
});
// Remaining arguments, if any, should be environment variables to set before invoking the server.
thisProcess.argv[6..].do({ |pairs|
    var splitPairs = pairs.split($=);
    "setting environment variable % to %".format(splitPairs[0], splitPairs[1]).postln;
    splitPairs[0].setenv(splitPairs[1]);
});
OSCFunc.trace(true);
quarkPath = thisProcess.argv[0];
versionMajor = thisProcess.argv[1].asInteger;
versionMinor = thisProcess.argv[2].asInteger;
versionPatch = thisProcess.argv[3].asInteger;
gitBranch = thisProcess.argv[4].asSymbol;
gitHash = thisProcess.argv[5].asSymbol;
options = ScinServerOptions.new;
options.quarkPath = quarkPath;
options.logLevel = 2;
options.createWindow = false;
options.frameRate = 0;
options.width = 200;
options.height = 200;
options.dumpOSC = true;
binPath = quarkPath +/+ "build/src/scinsynth";
fork {
	var commandLine, scinPid, udp, c, valid, response;
	var gotFrameRate, numWarn, numError, gotWarn, gotError;
	var awakeCount, awakeFunc;
	var testDef, echoFunc;
	commandLine = binPath + options.asOptionsString();
	"*** booting scinsynth with command line: %".format(commandLine).postln;
	scinPid = commandLine.unixCmd({ |exitCode, exitPid|
		if (valid, {
			// We'll exit this script with the same exit code as scinsynth provides here.
			"got scinsynth exit code %, terminating.".format(exitCode).postln;
			exitCode.exit;
		}, {
			"** didn't get response from /scin_quit".postln;
			1.exit;
		});
	});

	// We'll poll until we get a response.
	udp = NetAddr.new("127.0.0.1", options.portNumber);
	c = Condition.new;
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
	});

	udp.sendMsg('/scin_logAppend', 3, "Test Warning - ignore");
	numWarn = numWarn + 1;
	udp.sendMsg('/scin_logAppend', 4, "Test Error - ignore");
	numError = numError + 1;

	c.test = false;
	valid = false;
	response = OSCFunc.new({ |msg|
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
		1.exit;
	});

	// Now put a few workers to sleep and wait for sync again.
	c.test = false;
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
		awakeCount = awakeCount + 1;
	}, '/scin_awake');
	5.do({
		udp.sendMsg('/scin_sleepFor', 2);
	});
	udp.sendMsg('/scin_sync', 19);
	c.wait;
	if (valid.not or: { awakeCount != 5 }, {
		"*** /scin_sync test on busy queue failed!".postln;
		"expected 5 dones, got back %".format(awakeCount).postln;
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
			valid = false;
		});
		if (msg[6] != gitHash, {
			valid = false;
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
		1.exit;
	});

	"*** Testing /scin_d_recv".postln;
	c.test = false;
	valid = false;
	response = OSCFunc.new({ |msg|
		c.test = true;
		c.signal;
	}, '/scin_done').oneShot;
	echoFunc = OSCFunc.new({ |msg|
		if (msg[1] == 123, {
			valid = true;
		});
	}, '/scin_echoBack').oneShot;
	testDef = ScinthDef.new(\t, { BWOut.fg(1.0); });
	udp.sendMsg('/scin_d_recv', testDef.asYAML, ['/scin_echo', ['/scin_echoBack', 123].asRawOSC]);
	c.wait;
	if (valid.not, {
		"*** /scin_d_recv echo back not received.".postln;
		1.exit;
	});

	// We test quit last, should terminate the server but first send a final /scin_done message.
	valid = false;
	response = OSCFunc.new({ |msg|
		response.free;
		valid = true;
	}, '/scin_done');
	udp.sendMsg('/scin_quit');
}
)