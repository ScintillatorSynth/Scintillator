ScinServerInstaller {
	const releasesURL = "https://github.com/ScintillatorSynth/Scintillator/releases/download/";

	*install { |quarkPath|
		var version, binPath, downloadName, url, download;

		// Extract Scintillator version from Quark metadata.
		Quarks.installed.do({ |quark, index|
			if (quark.name == "Scintillator", {
				if (quarkPath.isNil, {
					quarkPath = quark.localPath;
				});
				version = quark.version;
			});
		});

		binPath = quarkPath +/+ "bin";

		// Build URL with platform-dependent download filename.
		Platform.case(
			\osx, {
				downloadName = "scinsynth.app.zip";
			},
			\linux, {
				downloadName = "scinsynth-x86_64.AppImage";
			},
			\windows, { Error.new("Windows not (yet) supported!").throw }
		);

		File.delete(binPath +/+ downloadName);

		url = releasesURL ++ version ++ "/" ++ downloadName;
		"Scintillator Server Installer downloading % server binary from %, saving to %".format(
			thisProcess.platform.name, url, binPath).postln;

		download = Download.new(url, binPath +/+ "foo", {
			ScinServerInstaller.prOnDownload(binPath)
		}, {
			ScinServerInstaller.prOnDownloadError;
		}, { |r, t|
			"  downloaded % / % bytes".format(r, t).postln;
		});
	}

	*prOnDownload { |binPath|
		"Scintillator binary download complete. Finalizing..".postln;

		Platform.case(
			\osx, {
				File.deleteAll(binPath +/+ "scinsynth.app");
			},
			\linux, {
				"chmod u+x %/scinsynth-x86_64.AppImage".format(binPath).unixCmd({ |exit, pid|
					if (exit == 0, {
						ScinServerInstaller.prOnComplete;
					}, {
						ScinServerInstaller.prOnError;
					});
				}, false);
			},
			\windows, { Error.new("Windows not (yet) supported!").throw }
		);
	}

	*prOnDownloadError {
		"*** error downloading Scintillator binary!".postln;
		"  Please double-check your Internet configuration. Failing that, try the manual "
		"install path detailed in the Scintillator User Guide in the help.".postln;
	}

	*prOnComplete {
		"*** Scintillator server download complete!".postln;
	}

	*prOnError {
		"*** error finalizing Scintillator server setup.".postln;
	}
}