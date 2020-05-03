ScinServerInstaller {
	const releasesURL = "https://scintillator-synth-coverage.s3-us-west-1.amazonaws.com/releases/";
	classvar routine;
	classvar continue;

	*initClass {
		Class.initClassTree(ScinServerOptions);
	}

	*setup { |cleanup=true, validate=true|
		if (routine.notNil, {
			"*** setup already running! Did you mean to call abort?".postln;
			^nil;
		});
		routine = {
			var version, quarkBinPath, binaryPath, downloadURL, downloadPath, runtime;
			var oldVersion, oldBinPath;
			var state = \init;
			continue = true;

			while ({ continue }, {
				switch (state,
					// Entry point for state machine. Setup variables.
					\init, {
						// Extract Scintillator version from Quark metadata.
						version = Scintillator.version;
						quarkBinPath = Scintillator.binPath;

						"*** ScinServerInstaller: checking installation..".postln;

						if (quarkBinPath.isNil, {
							"*** Unable to locate Scintillator Quark!".postln;
							"Something seems wrong with your Scintillator install. Try uninstalling the Quark "
							"and installing again.".postln;
							continue = false;
						});

						Platform.case(
							\osx, {
								binaryPath = quarkBinPath +/+ "scinsynth.app";
								downloadURL = releasesURL ++ version ++ "/scinsynth.app.zip";
								downloadPath = quarkBinPath +/+ "scinsynth.app." ++ version ++ ".zip";
								runtime = quarkBinPath +/+ "scinsynth.app/Contents/MacOS/scinsynth";
							},
							\linux, {
								binaryPath = quarkBinPath +/+ "scinsynth-x86_64.AppImage";
								downloadURL = releasesURL ++ version ++ "/scinsynth-x86_64.AppImage.gz";
								downloadPath = quarkBinPath +/+ "scinsynth-x86_64.AppImage." ++ version ++ ".gz";
								runtime = binaryPath;
							},
							\windows, {
								"*** Windows not (yet) supported!".postln;
								continue = false;
							}
						);

						state = \checkIfBinaryExists;
					},

					// Check if the scinserver binary already exists.
					\checkIfBinaryExists, {
						if (File.exists(binaryPath), {
							"scin installer detects pre-existing scinserver binary, checking version.".postln;
							state = \checkBinaryVersion;
						}, {
							"scin installer doesn't detect pre-existing scinserver binary, downloading.".postln;
							state = \checkIfDownloadExists;
						});
					},

					// If there'a already a binary version, try to run it to query version
					// string, to see if it matches the quark version.
					\checkBinaryVersion, {
						var prefix = "scinsynth version ";
						var scinOutput = "\"%\" --printVersion".format(runtime).unixCmdGetStdOut;
						if (scinOutput.beginsWith(prefix), {
							var split = scinOutput.split($ );
							var scinVersion = split[2];
							if (scinVersion == version, {
								"scin installer detects version match % between Quark and binary.".format(version).postln;
								if (cleanup, {
									if (downloadPath.notNil, {
										File.delete(downloadPath);
										File.delete(downloadPath ++ ".sha256");
									});
									if (oldBinPath.notNil, {
										File.deleteAll(oldBinPath);
									});
								});
								"*** You're all set, happy Scintillating!".postln;
								continue = false;
							}, {
								"version mismatch between Quark (%) and binary (%), downloading matching binary".format(
									version, scinOutput).postln;
								oldVersion = scinVersion;
								state = \checkIfDownloadExists;
							});
						}, {
							if (cleanup, {
								"*** error parsing scinserver version string, got back '%'. Deleting binary and redownloading."
								.format(scinOutput).postln;
								File.delete(binaryPath);
								state = \checkIfDownloadExists;
							}, {
								"*** error parsing version string from installed scinsynth binary. This shouldn't normally"
								"happen. Try manually deleting the file at %, or re-run this script with cleanup=true."
								"Aborting.".format(runtime).postln;
								continue = false;
							});
						});
					},

					\checkIfDownloadExists, {
						if (File.exists(downloadPath), {
							"detected existing download %, validating.".format(downloadPath).postln;
							if (validate, {
								state = \checkIfHashExists;
							}, {
								state = \extractBinary;
							});
						}, {
							"no pre-existing download found.".postln;
							state = \downloadBinary;
						});
					},

					\downloadBinary, {
						var result = false;
						var c = Condition.new;
						c.test = false;

						"starting download of scinserver binary from % to %".format(downloadURL, downloadPath).postln;
						Download.new(downloadURL, downloadPath, {
							result = true;
							c.test = true;
							c.signal;
						}, {
							result = false;
							c.test = true;
							c.signal;
						}, { |r, t|
							"  downloaded % of % KB".format((r / 1024).asInteger, (t / 1024).asInteger).postln;
						});

						c.wait;
						if (result, {
							if (validate, {
								state = \checkIfHashExists;
							}, {
								state = \extractBinary;
							});
						}, {
							"Scin server binary failed! Please check your Internet connection and try again.".postln;
							if (cleanup, {
								File.delete(downloadPath);
							});
							continue = false;
						});
					},

					\checkIfHashExists, {
						if (File.exists(downloadPath ++ ".sha256"), {
							"detected pre-existing hash file %, checking hash.".format(
								downloadPath ++ ".sha256").postln;
							state = \checkHash;
						}, {
							"no pre-existing hash file found.".postln;
							state = \downloadHashFile;
						});
					},

					\downloadHashFile, {
						var result = false;
						var c = Condition.new;
						c.test = false;
						"downloading hash file.".post;
						Download.new(downloadURL ++ ".sha256", downloadPath ++ ".sha256", {
							result = true;
							c.test = true;
							c.signal;
						}, {
							result = false;
							c.test = true;
							c.signal;
						}, {
						});

						c.wait;
						if (result, {
							"\nhashfile downloaded, checking against downloaded binary.".postln;
							state = \checkHash;
						}, {
							"\nScin hashfile download failed! Please check your Internet connection and try again.".postln;
							continue = false;
						});
					},

					\checkHash, {
						// TODO: windows has a different command for hashing
						var hashOutput = "shasum -a 256 -b \"%\"".format(downloadPath).unixCmdGetStdOut.split($ );
						var targetHash = File.readAllString(downloadPath ++ ".sha256").split($ );
						if (hashOutput[0] == targetHash[0], {
							"downloaded file validated, extracting.".postln;
							state = \extractBinary;
						}, {
							"*** hash mishmatch on downloaded file %. Expected '%', got '%'.".format(downloadPath,
								targetHash[0], hashOutput[0]).postln;
							if (cleanup, {
								"deleting bad hash file % and aborting. Please try again.".format(downloadPath).postln;
								File.delete(downloadPath);
								File.delete(downloadPath ++ ".sha256");
							}, {
								"bad download detected, aborting.".postln;
							});
							continue = false;
						});
					},

					\extractBinary, {
						// We have a validated compressed binary, move anything currenty there out of the way.
						if (File.exists(binaryPath), {
							state = \moveOldBinary;
						}, {
							Platform.case(
								\osx, {
									"unzip \"%\" -d \"%\"".format(downloadPath, quarkBinPath).unixCmdGetStdOut.postln;
									state = \checkIfBinaryExists;
								},
								\linux, {
									"gzip -d \"%\"".format(downloadPath).unixCmdGetStdOut.postln;
									"mv \"%\" \"%\"".format(downloadPath[0..downloadPath.size - 4],
										binaryPath).unixCmdGetStdOut.postln;
									"chmod u+x \"%\"".format(binaryPath).unixCmdGetStdOut.postln;
									state = \checkIfBinaryExists;
								},
								\windows, {
									continue = false;
								}
							);

						});
					},

					\moveOldBinary, {
						if (oldVersion.isNil, {
							"*** unable to determine version of old binary, using \"unknown\"".postln;
							oldVersion = ".unknown";
						});
						oldBinPath = binaryPath ++ "." ++ oldVersion ++ ".bak";
						"mv \"%\" \"%\"".format(binaryPath, oldBinPath).unixCmdGetStdOut;
						state = \extractBinary;
					},
				); // switch
			}); // while
		}.fork(AppClock);  // Download wants to run on the AppClock.
	} // *setup

	*abort {
		"*** aborting setup.".postln;
		continue = false;
		if (routine.notNil, {
			routine.stop;
		});
	}
}