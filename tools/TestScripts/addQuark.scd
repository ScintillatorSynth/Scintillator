// Loads the default language config, adds these quark classes to the include path list, and saves the resulting
// configuration in the /build directory, so future runs can include the Scintillator Quark.
(
var quarkPath;
if (thisProcess.argv.size != 1, {
    "requires one argument, the path to the Scintillator Quark root directory.".postln;
    -1.exit;
});

quarkPath = thisProcess.argv[0];

LanguageConfig.addIncludePath(quarkPath +/+ "classes");
LanguageConfig.store(quarkPath +/+ "build" +/+ "sclang_config.yaml");
0.exit;
)
