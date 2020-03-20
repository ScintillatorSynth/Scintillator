// Script to run the unit tests in the /tests object.
(
var exitCode = 0;
var c = Condition.new;
var d = Condition.new;
c.test = false;
d.test = false;
fork {

	var onFail = {
		exitCode = 1;
		c.test = true;
		c.signal;
	};
	d.test = true;
	d.signal;

	TestScinthDef.onFail_(onFail).reportPasses_(false).run;
	// Put more test objects here.

	c.test = true;
	c.signal;
};
fork {
	d.wait;
	c.wait;
    if (exitCode == 0, {
        "*** SCRIPT OK ***".postln;
    }, {
        "*** SCRIPT FAILED ***".postln;
    });
	exitCode.exit;
}
)
