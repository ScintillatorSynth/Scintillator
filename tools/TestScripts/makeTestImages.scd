// Generates a series of images for validation of individual VGens and integration testing of the render stack.
(
var quarkPath, imageOut, server, scinthDef, scinth;
if (thisProcess.argv.size != 1, {
    "requires one argument, the path to the Scintillator Quark root directory.".postln;
    -1.exit;
});
fork {
    quarkPath = thisProcess.argv[0];
    imageOut = quarkPath +/+ "build" +/+ "testing";
    server = ScinServer.new(5511, quarkPath, 0, false).boot;
    5.wait;
    server.screenShot(imageOut +/+ "firstFrame.png", "image/png");
    0.5.wait;
    server.advanceFrame(1, 10);
    0.5.wait;

    scinthDef = ScinthDef.new(\scinOsc, {
        RGBOut.fg(ScinOsc.fg(Length.fg(NormPos.fg)), ScinOsc.fg(0.5, VX.fg(NormPos.fg)),
            ScinOsc.fg(1.5, VY.fg(NormPos.fg)));
    }).add;
    1.0.wait;
    scinth = Scinth.new(\scinOsc, 1);
    server.screenShot(imageOut +/+ "scinOsc_a.png", "image/png");
    0.5.wait;
    server.advanceFrame(10, 1);
    0.5.wait;
    server.screenShot(imageOut +/+ "scinOsc_b.png", "image/png");
    0.5.wait;
    server.advanceFrame(10, 1);
    0.5.wait;
    server.screenShot(imageOut +/+ "scinOsc_c.png", "image/png");
    0.5.wait;
    server.advanceFrame(10, 1);
    0.5.wait;

    server.quit;
    1.wait;
    0.exit;
}
)
