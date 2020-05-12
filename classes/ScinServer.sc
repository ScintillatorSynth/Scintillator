Scintillator {
	*version {
		Quarks.installed.do({ |quark, index|
			if (quark.name == "Scintillator", {
				^quark.version;
			});
		});
		^nil;
	}
	*path {
		var p = PathName.new(Scintillator.class.filenameSymbol.asString.dirname).parentPath;
		if (thisProcess.platform.name === 'windows', {
			p = p.tr($/, $\\);
		});
		^p;
	}
	*binDir { ^Scintillator.path +/+ "bin" }
}

ScinServerOptions {
	classvar defaultValues;

	var <>portNumber;
	var <>dumpOSC;
	var <>frameRate;
	var <>createWindow;
	var <>logLevel;
	var <>width;
	var <>height;
	var <>alwaysOnTop;
	var <>swiftshader;
	var <>deviceName;
	var <>vulkanValidation;

	// Can install a function to call if server exits with a non-zero error code.
	var <>onServerError;

	*initClass {
		Class.initClassTree(Quarks);

		defaultValues = IdentityDictionary.newFrom(
			(
				portNumber: 5511,
				dumpOSC: false,
				frameRate: -1,
				createWindow: true,
				logLevel: 3,
				width: 800,
				height: 600,
				alwaysOnTop: true,
				swiftshader: false,
				deviceName: nil,
				vulkanValidation: false
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
		var o = "--quarkDir=";
		if (thisProcess.platform.name === 'windows', {
			o = o ++ Scintillator.path;
		}, {
			o = o ++ Scintillator.path.shellQuote;
		});

		if (portNumber != defaultValues[\portNumber], {
			o = o + "--portNumber=" ++ portNumber;
		});
		if (dumpOSC != defaultValues[\dumpOSC], {
			o = o + "--dumpOSC=" ++ dumpOSC;
		});
		if (frameRate != defaultValues[\frameRate], {
			o = o + "--frameRate=" ++ frameRate;
		});
		if (createWindow != defaultValues[\createWindow], {
			o = o + "--createWindow=" ++ createWindow;
		});
		if (logLevel != defaultValues[\logLevel], {
			o = o + "--logLevel=" ++ logLevel;
		});
		if (width != defaultValues[\width], {
			o = o + "--width=" ++ width;
		});
		if (height != defaultValues[\height], {
			o = o + "--height=" ++ height;
		});
		if (alwaysOnTop != defaultValues[\alwaysOnTop], {
			o = o + "--alwaysOnTop=" ++ alwaysOnTop;
		});
		if (swiftshader != defaultValues[\swiftshader], {
			o = o + "--swiftshader";
		});
		if (deviceName != defaultValues[\deviceName], {
			o = o + "--deviceName=" ++ deviceName;
		});
		if (vulkanValidation != defaultValues[\vulkanValidation], {
			o = o + "--vulkanValidation";
		});
		^o;
	}
}

ScinServer {
	classvar <>default;

	var <options;
	var scinBinaryPath;
	var scinPid;
	var addr;
	var statusPoller;

	// For local scinsynth instances. Remote not yet supported.
	*new { |options|
		^super.newCopyArgs(options).init;
	}

	*initClass {
		Class.initClassTree(ScinServerOptions);

		try {
			default = ScinServer();
		} { | e |
			"*** %".format(e.what).postln;
		};
	}

	init {
		if (options.isNil, {
			options = ScinServerOptions.new;
		});

		addr = NetAddr.new("127.0.0.1", options.portNumber);

		Platform.case(
			\osx, {
				scinBinaryPath = Scintillator.binDir +/+ "scinsynth.app" +/+ "Contents" +/+ "MacOS" +/+ "scinsynth";
				if (options.swiftshader and: { options.createWindow }, {
					Error.new("Swiftshader only supports offscreen render contexts." +
						"See https://github.com/ScintillatorSynth/Scintillator/issues/70.").throw;
				});
			},
			\linux, { scinBinaryPath = Scintillator.binDir +/+ "scinsynth-x86_64.AppImage" },
			\windows, { scinBinaryPath = Scintillator.binDir +/+ "scinsynth-w64" +/+ "scinsynth.exe" }
		);

		statusPoller = ScinServerStatusPoller.new(this);
	}

	boot {
		var commandLine;

		if (statusPoller.serverBooting or: { statusPoller.serverRunning }, {
			^this;
		});

		if (File.exists(scinBinaryPath).not, {
			Error.new("Unable to find Scintillator Server binary. Please run ScinServerInstaller.setup first.").throw;
			^nil;
		});

		statusPoller.serverBooting = true;
		if (thisProcess.platform.name === 'windows', {
			commandLine = scinBinaryPath + options.asOptionsString();
		}, {
			commandLine = scinBinaryPath.shellQuote + options.asOptionsString();
		});

		scinPid = commandLine.unixCmd({ |exitCode, exitPid|
			if (exitCode == 0, {
				"*** scinsynth exited normally.".postln;
			}, {
				statusPoller.serverBooting = false;
				"*** scinsynth fatal error, code: %".format(exitCode).postln;
				if (options.onServerError.notNil, {
					options.onServerError.value();
				});
			});
		});

	}

	quit {
		OSCFunc.new({
			// could do some interesting post quit stuff here.
		}, '/scin_done').oneShot;
		addr.sendMsg('/scin_quit');
		^this;
	}

	dumpOSC { |on|
		addr.sendMsg('/scin_dumpOSC', on.binaryValue);
		^this;
	}

	// Integer from 0 to 6.
	logLevel_ { |level|
		if (level >= 0 and: { level <= 6 }, {
			this.sendMsg('/scin_logLevel', level);
		});
		^this;
	}

	sendMsg { |... msg|
		addr.sendMsg(*msg)
		^this;
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
		^this;
    }

	advanceFrame { |num, denom|
		if (options.frameRate == 0) {
			this.sendMsg('/scin_nrt_advanceFrame', num, denom);
		}
		^this;
	}

	waitForBoot { |onComplete|
		this.doWhenBooted(onComplete);
		if (this.serverRunning.not, {
			this.boot;
		});
		^this;
	}

	doWhenBooted { |onComplete|
		statusPoller.doWhenBooted(onComplete);
		^this;
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
		^this;
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
		^this;
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
