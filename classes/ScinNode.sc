ScinNode {
	classvar <addActions;

	var <>nodeID;
	var <>server;
	var <>group;

	*initClass {
		addActions = (
			addToHead: 0,
			addToTail: 1,
			addBefore: 2,
			addAfter: 3,
			addReplace: 4,
			h: 0,
			t: 1,
			0: 0, 1: 1, 2: 2, 3: 3, 4: 4
		);
	}

	*basicNew { |server, nodeID|
		server = server ? ScinServer.default;
		^super.newCopyArgs(nodeID ?? { UniqueID.next }, server);
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

	moveBefore { |targetNode|
		group = targetNode.group;
		server.sendMsg('/scin_n_before', nodeID, targetNode.nodeID);
	}

	moveAfter { |targetNode|
		group = targetNode.group;
		server.sendMsg('/scin_n_after', nodeID, targetNode.nodeID);
	}

	moveToTail { |targetGroup|
		(targetGroup ? server.defaultGroup).moveNodeToHead(this);
	}

	moveToHead { |targetGroup|
		(targetGroup ? server.defaultGroup).moveNodeToTail(this);
	}

	asScinTarget { ^this; }
}

ScinGroup : ScinNode {
	*new { |target, addAction=\addToTail|
		var group, server, addActionID;
		target = target.asScinTarget;
		server = target.server;
		group = this.basicNew(server);
		addActionID = addActions[addAction];
		if (addActionID < 2, {
			group.group = target;
		}, {
			group.group = target.group;
		});
		server.sendMsg('/scin_g_new', group.nodeID, addActionID, target.nodeID);
		^group;
	}

	moveNodeToHead { |node|
		node.group = this;
		server.sendMsg('/scin_g_head', nodeID, node.nodeID);
	}

	moveNodeToTail { |node|
		node.group = this;
		server.sendMsg('/scin_g_tail', nodeID, node.nodeID);
	}

	freeAll {
		server.sendMsg('/scin_g_freeAll', nodeID);
	}

	deepFree {
		server.sendMsg('/scin_g_deepFree', nodeID);
	}

	dumpTree { |postControls=false|
		server.sendMsg('/scin_g_dumpTree', nodeID, postControls.binaryValue);
	}

	queryTree { |callback, includeControls=false|
		var query = OSCFunc.new({ |msg|
			if (msg[2] == nodeID, {
				query.free;
				callback.value(msg);
			});
		}, '/scin_g_queryTree.reply');
		server.sendMsg('/scin_g_queryTree', nodeID, includeControls.binaryValue);
	}
}

Scinth : ScinNode {
	var <>defName;

	*new { |defName, args, target, addAction=\addToTail|
		var scinth, server, addActionID;
		target = target.asScinTarget;
		server = target.server;
		addActionID = addActions[addAction];
		scinth = this.basicNew(defName, server);
		if (addActionID < 2, {
			scinth.group = target;
		}, {
			scinth.group = target.group;
		});
		server.sendMsg('/scin_s_new', defName, scinth.nodeID, addActionID,
			target.nodeID, *(args.asOSCArgArray));
		^scinth;
	}

	*basicNew { |defName, server, nodeID|
		^super.basicNew(server, nodeID).defName_(defName)
	}
}

+Nil {
	asScinTarget { ^ScinServer.default.asScinTarget; }
}

+Integer {
	asScinTarget { ^ScinGroup.basicNew(ScinServer.default, this); }
}