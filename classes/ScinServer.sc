ScinServer {
	classvar <>default;

	var udpPortNumber;
	var scinQuarkPath;
    var frameRate;
    var createWindow;
	var scinBinaryPath;
	var <logLevel;

	var scinQuarkVersion;
	var scinPid;
	var addr;

	// For local scinsynth instances. Remote not yet supported.
	*new { |udpPortNumber = 5511, scinQuarkPath = nil, frameRate = -1, createWindow = true|
		^super.newCopyArgs(udpPortNumber, scinQuarkPath, frameRate, createWindow).init;
	}

	init {
        if (scinQuarkPath.isNil, {
            Quarks.installed.do({ |quark, index|
                if (quark.name == "Scintillator", {
                    scinQuarkVersion = quark.version;
                    scinQuarkPath = quark.localPath;
                });
            });
        }, {
            scinQuarkVersion = "unknown";
        });

        scinBinaryPath = scinQuarkPath +/+ "build/src/scinsynth";
		logLevel = 2;
	}

	boot {
		var commandLine = scinBinaryPath + "--udp_port_number=" ++ udpPortNumber.asString()
		+ "--quark_dir=" ++ scinQuarkPath + "--log_level=" ++ logLevel.asString() + "--frame_rate="
        ++ frameRate.asString() + "--create_window=" ++ createWindow.asString();
		commandLine.postln;

		scinPid = commandLine.unixCmd({ |exitCode, exitPid|
			"*** got scinsynth exit code %".format(exitCode).postln;
		});

		if (ScinServer.default.isNil, {
			ScinServer.default = this;
		});
		addr = NetAddr.new("127.0.0.1", udpPortNumber);
		^this;
	}

	status {
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
			logLevel = level;
			addr.sendMsg('/scin_logLevel', logLevel);
		});
	}

	sendMsg { |... msg|
		addr.sendMsg(*msg)
	}

	prGetVersionAsync { |callback|
		fork {
			var sync = Condition.new;
			var version;
			OSCFunc.new({ |msg|
				version = msg[2].asString;
				version = version ++ "." ++ msg[3].asString;
				version = version ++ "." ++ msg[4].asString;
				version = version + msg[5] ++ "@" ++ msg[6];

				sync.test = true;
				sync.signal;
			}, '/scin_version.reply').oneShot;
			addr.sendMsg('/scin_version');
			sync.wait;
			callback.value(version);
		}
	}
}
