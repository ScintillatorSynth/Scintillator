// Loads the default language config, adds these quark classes to the include path list, and saves the resulting
// configuration in the /build directory, so future runs can include the Scintillator Quark.
(
var quarkPath;
if (thisProcess.argv.size != 1, {
    "requires one argument, the path to the Scintillator Quark root directory.".postln;
    -1.exit;
});

quarkPath = thisProcess.argv[0];

fork {
    // TODO: remove workaround for https://github.com/supercollider/supercollider/issues/3393
    1.sleep;
    LanguageConfig.addIncludePath(quarkPath +/+ "classes");
    LanguageConfig.addIncludePath(quarkPath +/+ "tests");
    LanguageConfig.store(quarkPath +/+ "build" +/+ "testing" +/+ "sclang_config.yaml");
    "*** SCRIPT OK ***".postln;
    0.exit;
}
)
