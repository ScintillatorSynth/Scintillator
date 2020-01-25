#!/usr/bin/python3

import datetime
import os
import shutil
import subprocess
import sys
import yaml

def main(argv):
    if len(argv) != 4:
        print('compare-test-images.py <Scintillator path> <TestGoldImages path> <commithash> <output directory path>');
        sys.exit(2)
    scinPath = argv[0]
    goldPath = argv[1]
    commitHash = argv[2]
    outDir = argv[3]

    # build the output directory and start the report html file.
    os.makedirs(outDir, exist_ok=True)
    outFile = open(os.path.join(outDir, "report.html"), 'w')
    outFile.write("""<html>
<head>
<title>Scintillator Image Test Report</title>
</head>
<body>
<h1>Scintillator Image Test Report</h3>
<h2>Revision {commitHash} on {timestamp}</h4>
""".format(commitHash=commitHash, timestamp=datetime.datetime.now().strftime("%c")))

    manifestFile = open(os.path.join(scinPath, "tools", "TestScripts", "testManifest.yaml"), 'r')
    manifest = yaml.safe_load(manifestFile)
    manifestFile.close()

    category = ""
    diffCount = 0
    for item in manifest:
        if item['category'] != category:
            category = item['category']
            refOutDir = os.path.join(outDir, "images", "ref", category)
            os.makedirs(refOutDir, exist_ok=True)
            testOutDir = os.path.join(outDir, "images", "test", category)
            os.makedirs(testOutDir, exist_ok=True)
            diffOutDir = os.path.join(outDir, "images", "diff", category)
            os.makedirs(diffOutDir, exist_ok=True)
            outFile.write("""
<hr/>
<h3>{category}</h3>
""".format(category=category))
        outFile.write("""
<h4>{name}</h4>
<p>{comment}</p>
<p>ScinthDef source: <pre>{scinthDef}</pre></p>
<table>
<tr><th>status</th><th>Scinth time (s)</th><th>reference image</th><th>{commitHash} image</th><th>difference</th></tr>
""".format(name=item['name'], comment=item['comment'], scinthDef=item['scinthDef'], commitHash=commitHash))
        t = 0
        for dt in item['captureTimes']:
            imageFileName = item['shortName'] + '_' + str(t) + '.png'
            # copy reference file to report output directory
            shutil.copyfile(os.path.join(goldPath, "reference", category, imageFileName),
                    os.path.join(refOutDir, imageFileName))
            # copy test image file to report output directory
            shutil.copyfile(os.path.join(scinPath, "build", "testing", category, imageFileName),
                    os.path.join(testOutDir, imageFileName))
            # compute difference image
            subprocess.run(["compare",
                os.path.join(refOutDir, imageFileName),
                os.path.join(testOutDir, imageFileName),
                os.path.join(diffOutDir, imageFileName)])
            # now capture result from separate computation TODO: is there a way to get both from one run of compare?
            results = subprocess.run(["compare",
                os.path.join(refOutDir, imageFileName),
                os.path.join(testOutDir, imageFileName),
                "-metric", "PSNR", "-format", "'%[fx:mean*100]'", "info:"],
                capture_output=True, encoding="utf-8")
            status = 'OK'
            if results.stderr != 'inf':
                status = '<strong>DIFERENT</strong>'
                diffCount += 1
                print("difference detected in {category}/{filename}".format(category=category, filename=imageFileName))
            # write out table row
            outFile.write("""
<tr><td>{status}</td><td>{t}</td><td><img src="images/ref/{category}/{name}" /></td><td><img src="images/test/{category}/{name} " /></td><td><img src="images/diff/{category}/{name}" /></td></tr>""".format(
    status=status, t=t, category=category, name=imageFileName))
            t += dt

        outFile.write("""
</table>""")

    outFile.write("""
</body>
</html>""")
    outFile.close()
    sys.exit(diffCount)

if __name__ == "__main__":
    main(sys.argv[1:])

