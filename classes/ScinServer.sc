ScinServerOptions {
	classvar defaultValues;

	var <>quarkPath;
	var <>udpPortNumber;
	var <>frameRate;
	var <>createWindow;
	var <>logLevel;
	var <>width;
	var <>height;
	var <>keepOnTop;

	*initClass {
		defaultValues = IdentityDictionary.newFrom(
			(
				quarkPath: nil,
				udpPortNumber: 5511,
				frameRate: -1,
				createWindow: true,
				logLevel: 3,
				width: 800,
				height: 600,
				keepOnTop: true,
			)
		);
	}

	*new {
		^super.new.init;
	}

	init {
		defaultValues.keysValuesDo({ |key, value| this.instVarPut(key, value); });
	}

	asOptionsString {
		var o = "--quark_dir=" ++ quarkPath;
		if (udpPortNumber != defaultValues[\udpPortNumber], {
			o = o + "--udp_port_number=" ++ udpPortNumber;
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

        scinBinaryPath = options.quarkPath +/+ "build/src/scinsynth";
		statusPoller = ScinServerStatusPoller.new(this);
	}

	boot {
		var commandLine = scinBinaryPath + options.asOptionsString();
		commandLine.postln;

		scinPid = commandLine.unixCmd({ |exitCode, exitPid|
			"*** got scinsynth exit code %".format(exitCode).postln;
		});

		if (ScinServer.default.isNil, {
			ScinServer.default = this;
		});
		addr = NetAddr.new("127.0.0.1", options.udpPortNumber);
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

    screenShot { |fileName, mimeType|
        if (options.frameRate >= 0) {
            this.sendMsg('/scin_nrt_screenShot', fileName, mimeType);
        }
    }

	advanceFrame { |num, denom|
		if (options.frameRate == 0) {
			this.sendMsg('/scin_nrt_advanceFrame', num, denom);
		}
	}

	// Call on Routine
	sync {
		var condition = Condition.new;
		var id = UniqueID.next;
		var response = OSCFunc.new({ |msg|
			if (msg[1] == id, {
				response.free;
				condition.test = true;
				condition.signal;
			});
		}, '/scin_synced');
		condition.test = false;
		this.sendMsg('/scin_sync', id);
		condition.wait;
	}
}
