ScinServerStatusPoller {
	var scinServer;
	var statusReplyFunc;
	var shouldStop;
	var pingTask;

	var lastPingReceived;

	var <isRunning;
	var <numberOfScinths;
	var <numberOfGroups;
	var <numberOfScinthDefs;
	var <numberOfWarnings;
	var <numberOfErrors;
	var <graphicsBytesUsed;
	var <graphicsBytesTotal;
	var <targetFrameRate;
	var <meanFrameRate;
	var <numberOfDroppedFrames;

	*new { |scinServer|
		^super.newCopyArgs(scinServer).init;
	}

	init {
		isRunning = false;
		statusReplyFunc = OSCFunc.new({ |msg, time, addr|
			lastPingReceived = time;
			numberOfScinths = msg[1];
			numberOfGroups = msg[2];
			numberOfScinthDefs = msg[3];
			numberOfWarnings = msg[4];
			numberOfErrors = msg[5];
			graphicsBytesUsed = msg[6];
			graphicsBytesTotal = msg[7];
			targetFrameRate = msg[8];
			meanFrameRate = msg[9];
			numberOfDroppedFrames = msg[10];
		},
		'/scin_status.reply');

		shouldStop = false;
		pingTask = SkipJack.new({
			var now = Main.elapsedTime;
			if (lastPingReceived.notNil and: { now - lastPingReceived < 1.0 }, {
				if (isRunning.not, {
					isRunning = true;
					this.prOnRunningStart;
				});
			}, {
				if (isRunning, {
					isRunning = false;
					this.prOnRunningStop;
				});
			});
			scinServer.sendMsg('/scin_status');
		},
		dt: 0.5,
		stopTest: { shouldStop },
		name: "ScinServerStatusPoller");
	}

	prOnRunningStart {
	}

	prOnRunningStop {
		// Zero out variables except for errors and warnings, which can be
		// reset by new server instance.
		numberOfScinths = 0;
		numberOfGroups = 0;
		numberOfScinthDefs = 0;
		graphicsBytesUsed = 0;
		graphicsBytesTotal = 0;
		targetFrameRate = 0;
		meanFrameRate = 0;
		numberOfDroppedFrames = 0;L
	}
}