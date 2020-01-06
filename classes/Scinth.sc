Scinth {
	var <>defName;
	var <>nodeID;
	var <>server;

	*new { |defName, nodeID, server|
		server = server ? ScinServer.default;
		nodeID = (nodeID ? -1).asInteger;
		server.sendMsg('/scin_s_new', defName, nodeID);
		^super.newCopyArgs(defName, nodeID, server);
	}

	free {
		server.sendMsg('/scin_n_free', nodeID);
	}

	run { |flag=true|
		server.sendMsg('/scin_n_run', nodeID, flag.binaryValue);
	}
}