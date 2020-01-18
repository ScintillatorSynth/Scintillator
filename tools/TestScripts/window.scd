// A standalone smoke test for Scintillator, opens a new Scintillator window, adds a very simple test ScinthDef,
// renders it for a few seconds, then quits. Expects the path to the quark as the only argument.
(
var quarkPath, server, scinthDef, scinth;
if (thisProcess.argv.size != 1, {
    "requires one argument, the path to the Scintillator Quark root directory.".postln;
    -1.exit;
});
fork {
    quarkPath = thisProcess.argv[0];
    server = ScinServer.new(5511, quarkPath).boot;
    5.wait;  // TODO: add onBoot
    scinthDef = ScinthDef.new(\a, { BWOut.fg(ScinOsc.fg(0.1, Length.fg(NormPos.fg))); }).add;
    5.wait;  // TODO: isn't there a callback supported on add?
    scinth = Scinth.new(\a);
    5.wait;
    server.quit;
    0.exit;
}
)
