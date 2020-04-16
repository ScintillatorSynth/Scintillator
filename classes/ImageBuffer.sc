ImageBuffer {
	var <server, <bufnum;
	var <width, <height;
	var action;
	var queryFunc;

	*read { |server, path, width, height, action, bufnum|
		server = server ? ScinServer.default;
		bufnum ?? { bufnum = UniqueID.next };
		width = width ? -1;
		height = height ? - 1;
		^super.newCopyArgs(server, bufnum, width, height, action)
		.bindQuery()
		.allocRead(path, width, height, ['/scin_ib_query', bufnum]);
	}

	bindQuery {
		if (queryFunc.isNil, {
			queryFunc = OSCFunc.new({ |msg|
				if (msg[1] == bufnum, {
					queryFunc.free;
					width = msg[3];
					height = msg[4];
					if (action.notNil, {
						action.value(this);
					});
				});
			}, '/scin_ib_info');
		});
	}

	// Buffer completion is a function, ours is just a message array. Currently this
	// method is marked as "private" in the documentation, because we do not support
	// allocating empty buffers on the server currently.
	allocRead { |path, width, height, completion|
		width = width ? -1;
		height = height ? -1;
		server.sendMsg('/scin_ib_allocRead', bufnum, path, width, height, completion);
	}

	asVGenInput {
		^this.bufnum;
	}
}
