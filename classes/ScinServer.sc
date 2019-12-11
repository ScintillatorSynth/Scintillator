ScinServer {
	var udpPortNumber;
	var scinBinaryPath;

	var scinPid;
	var addr;

	// For local scinsynth instances. Remote not yet supported.
	*new { |udpPortNumber = 52210, scinBinaryPath|
		^super.newCopyArgs(udpPortNumber, scinBinaryPath).init;
	}

	init {
		// If no override path specificed we assume the Scintillator binary is in the bin/ path
		// within the Quark.
		if (scinBinaryPath.isNil, {
			Quarks.installed.do({ |quark, index|
				if (quark.name == "Scintillator", {
					scinBinaryPath = quark.localPath ++ "/build/src/scinsynth";
				});
			});
		});
	}

	boot {
		var commandLine = scinBinaryPath + "--udp_port_number=" ++ udpPortNumber.asString();
		commandLine.postln;
		scinPid = commandLine.unixCmd({ |exitCode, exitPid|
			"*** got scinsynth exit code %".format(exitCode).postln;
		});

		addr = NetAddr.new("127.0.0.1", udpPortNumber);
	}

	status {
	}

	quit {
		OSCFunc.new({
			"*** got scin_done back".postln;
		}, '/scin_done').oneShot;
		addr.sendMsg('/scin_quit');
	}
}
