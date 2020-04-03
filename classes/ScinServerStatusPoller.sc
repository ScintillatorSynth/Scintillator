ScinServerStatusPoller {
	var scinServer;
	var pingPeriod;
	var statusReplyFunc;
	var shouldStop;
	var pingTask;

	var lastPingReceived;
	var bootCallbacks;

	var <serverRunning;
	var <>serverBooting;
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

	*new { |scinServer, pingPeriod=1.0|
		^super.newCopyArgs(scinServer, pingPeriod).init;
	}

	init {
		bootCallbacks = List.new;
		serverRunning = false;
		serverBooting = false;
		lastPingReceived = false;
		statusReplyFunc = OSCFunc.new({ |msg, time, addr|
			lastPingReceived = true;
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
			if (lastPingReceived, {
				if (serverRunning.not, {
					serverRunning = true;
					serverBooting = false;
					this.prOnRunningStart;
				});
			}, {
				if (serverRunning, {
					serverRunning = false;
					this.prOnRunningStop;
				});
			});
			lastPingReceived = false;
			scinServer.sendMsg('/scin_status');
		},
		dt: pingPeriod,
		stopTest: { shouldStop },
		name: "ScinServerStatusPoller");
	}

	// adds onComplete to functions to call when the server boot is detected. Or calls
	// immediately if the server is already booted.
	doWhenBooted { |onComplete|
		if (serverRunning, {
			onComplete.value();
		}, {
			bootCallbacks.add(onComplete);
		});
	}

	prOnRunningStart {
		fork {
			scinServer.sync;
			bootCallbacks.do({ |callback| callback.value(scinServer); });
			bootCallbacks.clear();
		}
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
		numberOfDroppedFrames = 0;
	}
}