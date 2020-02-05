// Script to run the unit tests in the /tests object.
(
var exitCode = 0;
var c = Condition.new;
c.test = false;
fork {
	var onFail = {
		exitCode = 1;
		c.test = true;
		c.signal;
	};
	TestScinthDef.onFail_(onFail).reportPasses_(false).run;
	// Put more test objects here.
	c.test = true;
	c.signal;
};
fork {
		c.wait;
		exitCode.exit;
}
)