Scinth {
	var <>defName;
	var <nodeID;
	var <server;

	*new { |defName, args, server|
		var nodeID = UniqueID.next;
		server = server ? ScinServer.default;
		if (args.notNil, {
			server.sendMsg('/scin_s_new', defName, nodeID, 0, 0, *args);
		}, {
			server.sendMsg('/scin_s_new', defName, nodeID);
		});
		^super.newCopyArgs(defName, nodeID, server);
	}

	free {
		server.sendMsg('/scin_n_free', nodeID);
	}

	run { |flag=true|
		server.sendMsg('/scin_n_run', nodeID, flag.binaryValue);
		^this;
	}

	set { |...args|
		server.sendMsg('/scin_n_set', nodeID, *args);
		^this;
	}
}