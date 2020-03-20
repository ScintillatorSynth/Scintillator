ScinServerOptions {
	classvar defaultValues;

	var <>quarkPath;
	var <>portNumber;
	var <>dumpOSC;
	var <>frameRate;
	var <>createWindow;
	var <>logLevel;
	var <>width;
	var <>height;
	var <>keepOnTop;
	var <>swiftshader;
	var <>deviceName;

	// Can install a function to call if server exits with a non-zero error code.
	var <>onServerError;

	*initClass {
		defaultValues = IdentityDictionary.newFrom(
			(
				quarkPath: nil,
				portNumber: 5511,
				dumpOSC: false,
				frameRate: -1,
				createWindow: true,
				logLevel: 3,
				width: 800,
				height: 600,
				keepOnTop: true,
				swiftshader: false,
				deviceName: nil
			)
		);
	}

	*new {
		^super.new.init;
	}

	init {
		defaultValues.keysValuesDo({ |key, value| this.instVarPut(key, value); });
		onServerError = {};
	}

	asOptionsString {
		var o = "--quark_dir=" ++ quarkPath;
		if (portNumber != defaultValues[\portNumber], {
			o = o + "--port_number=" ++ portNumber;
		});
		if (dumpOSC != defaultValues[\dumpOSC], {
			o = o + "--dump_osc=" ++ dumpOSC;
		});
		if (frameRate != defaultValues[\frameRate], {
			o = o + "--frame_rate=" ++ frameRate;
		});
		if (createWindow != defaultValues[\createWindow], {
			o = o + "--create_window=" ++ createWindow;
		});
		if (logLevel != defaultValues[\logLevel], {
			o = o + "--log_level=" ++ logLevel;
		});
		if (width != defaultValues[\width], {
			o = o + "--width=" ++ width;
		});
		if (height != defaultValues[\height], {
			o = o + "--height=" ++ height;
		});
		if (keepOnTop != defaultValues[\keepOnTop], {
			o = o + "--keep_on_top=" ++ keepOnTop;
		});
		if (swiftshader != defaultValues[\swiftshader], {
			o = o + "--swiftshader=" ++ swiftshader;
		});
		if (deviceName != defaultValues[\deviceName], {
			o = o + "--device_name=" ++ deviceName;
		});
		^o;
	}
}

ScinServer {
	classvar <>default;

	var options;
	var scinBinaryPath;
	var scinQuarkVersion;
	var scinPid;
	var addr;
	var statusPoller;

	// For local scinsynth instances. Remote not yet supported.
	*new { |options|
		^super.newCopyArgs(options).init;
	}

	init {
		if (options.isNil, {
			options = ScinServerOptions.new;
		});
        if (options.quarkPath.isNil, {
            Quarks.installed.do({ |quark, index|
                if (quark.name == "Scintillator", {
                    scinQuarkVersion = quark.version;
                    options.quarkPath = quark.localPath;
                });
            });
        }, {
            scinQuarkVersion = "unknown";
        });

		scinBinaryPath = options.quarkPath +/+ "bin";
		Platform.case(
			\osx, {
				scinBinaryPath = scinBinaryPath +/+ "scinsynth.app" +/+ "Contents" +/+ "MacOS" +/+ "scinsynth";
				if (options.swiftshader and: { options.createWindow }, {
					Error.new("Swiftshader only supports offscreen render contexts." +
						"See https://github.com/ScintillatorSynth/Scintillator/issues/70.").throw;
				});
			},
			\linux, { scinBinaryPath = scinBinaryPath +/+ "scinsynth" },
			\windows, { Error.new("Windows not (yet) supported!").throw }
		);
		statusPoller = ScinServerStatusPoller.new(this);
	}

	boot {
		var commandLine;

		if (statusPoller.serverBooting or: { statusPoller.serverRunning }, {
			^this;
		});

		statusPoller.serverBooting = true;
		commandLine = scinBinaryPath + options.asOptionsString();
		commandLine.postln;

		scinPid = commandLine.unixCmd({ |exitCode, exitPid|
			"*** got scinsynth exit code %".format(exitCode).postln;
			if (exitCode != 0, {
				options.onServerError.value(exitCode);
			});
		});

		if (ScinServer.default.isNil, {
			ScinServer.default = this;
		});
		addr = NetAddr.new("127.0.0.1", options.portNumber);
		^this;
	}

	quit {
		OSCFunc.new({
			// could do some interesting post quit stuff here.
		}, '/scin_done').oneShot;
		addr.sendMsg('/scin_quit');
	}

	dumpOSC { |on|
		addr.sendMsg('/scin_dumpOSC', on.binaryValue);
	}

	// Integer from 0 to 6.
	logLevel_ { |level|
		if (level >= 0 and: { level <= 6 }, {
			this.sendMsg('/scin_logLevel', level);
		});
	}

	sendMsg { |... msg|
		addr.sendMsg(*msg)
	}

    screenShot { |fileName, mimeType, onReady, onComplete|
		var ready, complete;
		if (options.frameRate < 0, {
			"screenShot only supported on non-realtime rendering modes.".postln;
			^nil;
		});

		if (onReady.notNil, {
			ready = OSCFunc.new({ |msg|
				if (msg[1] == fileName.asSymbol, {
					ready.free;
					onReady.value(msg[2]);
				});
			}, '/scin_nrt_screenShot.ready');
		});
		if (onComplete.notNil, {
			complete = OSCFunc.new({ |msg|
				if (msg[1] == '/scin_nrt_screenShot' and: {
					msg[2] == fileName.asSymbol }, {
					complete.free;
					onComplete.value(msg[3]);
				});
			}, '/scin_done');
		});
        this.sendMsg('/scin_nrt_screenShot', fileName, mimeType);
    }

	advanceFrame { |num, denom|
		if (options.frameRate == 0) {
			this.sendMsg('/scin_nrt_advanceFrame', num, denom);
		}
	}

	waitForBoot { |onComplete|
		this.doWhenBooted(onComplete);
		if (this.serverRunning.not, {
			this.boot;
		});
	}

	doWhenBooted { |onComplete|
		statusPoller.doWhenBooted(onComplete);
	}

	// Call on Routine
	bootSync { |condition|
		condition ?? { condition = Condition.new };
		condition.test = false;
		this.waitForBoot({
			condition.test = true;
			condition.signal;
		});
		condition.wait;
	}

	// Call on Routine
	sync { |condition|
		var id, response;
		condition ?? { condition = Condition.new };
		condition.test = false;
		id = UniqueID.next;
		response = OSCFunc.new({ |msg|
			if (msg[1] == id, {
				response.free;
				condition.test = true;
				condition.signal;
			});
		}, '/scin_synced');
		this.sendMsg('/scin_sync', id);
		condition.wait;
	}

	// Call on Routine, blocks until the screen shot is queued, then returns status
	// code of the queue.
	queueScreenShotSync { |fileName, mimeType, onComplete, condition|
		var result;
		condition ?? { condition = Condition.new };
		condition.test = false;
		this.screenShot(fileName, mimeType, { |r|
			result = r;
			condition.test = true;
			condition.signal;
		}, onComplete);
		condition.wait;
		^result;
	}

	serverRunning { ^statusPoller.serverRunning; }
	serverBooting { ^statusPoller.serverBooting; }
	numberOfWarnings { ^statusPoller.numberOfWarnings; }
	numberOfErrors { ^statusPoller.numberOfErrors; }
}
