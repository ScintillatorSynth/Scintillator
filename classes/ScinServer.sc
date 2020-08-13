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
	var <>audioInputChannels;
	var <>crashReporting;

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
				vulkanValidation: false,
				audioInputChannels: 0,
				crashReporting: true
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
		if (audioInputChannels != defaultValues[\audioInputChannels], {
			o = o + "--audioInputChannels=" ++ audioInputChannels;
		});
		if (crashReporting != defaultValues[\crashReporting], {
			o = o + "--crashReporting=" ++ crashReporting;
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
	var currentLogLevel;
	var <defaultGroup;

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
		CmdPeriod.add(this);
		ScinServer.default = this;

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

	cmdPeriod {
		if (addr.notNil, {
			addr.sendMsg('/scin_g_freeAll', defaultGroup.nodeID);
		});
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

		statusPoller.doWhenBooted({ this.prMakeDefaultGroup; });
		statusPoller.serverBooting = true;

		Platform.case(
			\osx, {
				commandLine = scinBinaryPath.shellQuote + options.asOptionsString() + '--crashpadHandlerPath='
				++ (Scintillator.binDir +/+ "scinsynth.app" +/+ "Contents" +/+ "MacOS" +/+ "crashpad_handler").shellQuote;
			},
			\linux, {
				commandLine = scinBinaryPath.shellQuote + options.asOptionsString();
			},
			\windows, {
				commandLine = scinBinaryPath + options.asOptionsString() + '--crashpadHandlerPath='
				++ Scintillator.binDir +/+ "scinsynth-w64" +/+ "crashpad_handler.com";
			}
		);

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

		currentLogLevel = options.logLevel;
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
			currentLogLevel = level;
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
					var status = msg[2];
					// Older versions of sclang do not cast 'T' and 'F' to booleans.
					if (status.class === $T.class, {
						status = (status == $T);
					});
					ready.free;
					onReady.value(status);
				});
			}, '/scin_nrt_screenShot.ready');
		});
		if (onComplete.notNil, {
			complete = OSCFunc.new({ |msg|
				if (msg[1] == '/scin_nrt_screenShot' and: {
					msg[2] == fileName.asSymbol }, {
					var status = msg[3];
					if (status.class === $T.class, {
						status = (status == $T);
					});
					complete.free;
					onComplete.value(status);
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

	postCrashReports {
		// Temporarily lower the log level to see the reports, an inelegant solution
		// that might reveal other log spam but we want folks to be able to see
		// the report UUIDs and upload crash reports without too much fuss.
		this.prTempLowerLogLevel(2, '/scin_logCrashReports');
		this.sendMsg('/scin_logCrashReports');
	}

	uploadCrashReport { |id|
		this.prTempLowerLogLevel(2, '/scin_uploadCrashReport');
		this.sendMsg('/scin_uploadCrashReport', id);
	}

	uploadAllCrashReports {
		this.prTempLowerLogLevel(2, '/scin_uploadCrashReport');
		this.sendMsg('/scin_uploadCrashReport', 'all');
	}

	reorder { |nodeList, target, addAction=\addToHead|
		target = target.asScinTarget;
		this.sendMsg('/scin_n_order', ScinNode.actionNumberFor(addAction), target.nodeID, *(nodeList.collect(_.nodeID)));
	}

	prTempLowerLogLevel { |newLevel, doneToken|
		if (currentLogLevel > newLevel, {
			var oldLevel = currentLogLevel;
			var complete = OSCFunc.new({ |msg|
				if (msg[1] == doneToken, {
					complete.free;
					this.logLevel = oldLevel;
				});
			}, '/scin_done');
			this.logLevel = newLevel;
		});
	}

	prMakeDefaultGroup {
		defaultGroup = ScinGroup.basicNew(this);
		this.sendMsg('/scin_g_new', defaultGroup.nodeID, 0, 0);
	}

	serverRunning { ^statusPoller.serverRunning; }
	serverBooting { ^statusPoller.serverBooting; }
	numberOfWarnings { ^statusPoller.numberOfWarnings; }
	numberOfErrors { ^statusPoller.numberOfErrors; }
	asScinTarget { ^this.defaultGroup; }
}
