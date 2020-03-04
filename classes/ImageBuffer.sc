ImageBuffer {
	var <server, <bufnum;
	var <width, <height;
	var action;

	*read { |server, path, width, height, action, bufnum|
		server = server ? ScinServer.default;
		bufnum ?? { bufnum = UniqueID.next };
		width = width ? -1;
		height = height ? - 1;
		^super.newCopyArgs(server, bufnum, width, height, action)
		.allocRead(path, width, height, ['/scin_ib_query', bufnum]);
	}

	// Buffer completion is a function, ours is just a message array
	allocRead { |path, width, height, completion|
		width = width ? -1;
		height = height ? -1;
		server.sendMsg('/scin_ib_allocRead', bufnum, path, width, height, completion);
	}
}