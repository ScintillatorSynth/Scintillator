ScinServer {
	classvar <>default;

	var udpPortNumber;
	var scinBinaryPath;
	var scinQuarkPath;
	var logLevel;

	var scinQuarkVersion;
	var scinPid;
	var addr;

	// For local scinsynth instances. Remote not yet supported.
	*new { |udpPortNumber = 5511, scinBinaryPath|
		^super.newCopyArgs(udpPortNumber, scinBinaryPath).init;
	}

	init {
		Quarks.installed.do({ |quark, index|
			if (quark.name == "Scintillator", {
				// If no override path specificed we assume the Scintillator binary is in the
				// bin/ path within the Quark.
				if (scinBinaryPath.isNil, {
					scinBinaryPath = quark.localPath ++ "/build/src/scinsynth";
				});
				scinQuarkVersion = quark.version;
				scinQuarkPath = quark.localPath;
			});
		});

		logLevel = 2;
	}

	boot {
		var commandLine = scinBinaryPath + "--udp_port_number=" ++ udpPortNumber.asString()
		+ "--quark_dir=" ++ scinQuarkPath + "--log_level=" ++ logLevel.asString();
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
			"*** got scin_done back".postln;
		}, '/scin_done').oneShot;
		addr.sendMsg('/scin_quit');
	}

	dumpOSC { |on|
		if (on, {
			addr.sendMsg('/scin_dumpOSC', 1);
		}, {
			addr.sendMsg('/scin_dumpOSC', 0);
		});
	}

	// Integer from 0 to 6.
	logLevel { |level|
		if (level >= 0 and: { level <= 6 }, {
			addr.sendMsg('/scin_logLevel', level);
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
