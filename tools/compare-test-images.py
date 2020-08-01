#!/usr/bin/python3

import datetime
import os
import shutil
import subprocess
import sys
import yaml

def main(argv):
    if len(argv) != 5:
        print('compare-test-images.py <Scintillator path> <TestGoldImages path> <commithash> <test image hash> <output directory path>');
        sys.exit(2)
    if sys.version_info.major != 3:
        print('requires Python 3')
        sys.exit(2)

    scinPath = argv[0]
    goldPath = argv[1]
    commitHash = argv[2]
    testImageHash = argv[3]
    outDir = argv[4]

    # build the output directory and start the report html file.
    os.makedirs(outDir, exist_ok=True)
    outFile = open(os.path.join(outDir, "report.html"), 'w')
    outFile.write("""<html>
<head>
<title>Scintillator Image Test Report</title>
</head>
<body>
<h1>Scintillator Image Test Report</h1>
<h2>Revision {commitHash} on {timestamp}</h2>
<h2>Test Image Revision {testImageHash}</h2>
""".format(commitHash=commitHash, timestamp=datetime.datetime.now().strftime("%c"), testImageHash=testImageHash))

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
        scinthDef = 'none specified'
        if 'scinthDef' in item:
            scinthDef = item['scinthDef']
        outFile.write("""
<h4>{name}</h4>
<p>{comment}</p>
<p>ScinthDef source: <pre>{scinthDef}</pre></p>
<table>
<tr><th>status</th><th>Scinth time (s)</th><th>reference image</th><th>{commitHash} image</th><th>difference</th></tr>
""".format(name=item['name'], comment=item['comment'], scinthDef=scinthDef, commitHash=commitHash))
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
            if sys.version_info.minor > 5:
                results = subprocess.run(["compare",
                    os.path.join(refOutDir, imageFileName),
                    os.path.join(testOutDir, imageFileName),
                    "-metric", "PSNR", "-format", "'%[fx:mean*100]'", "info:"],
                    encoding="utf-8", stderr=subprocess.PIPE)
                out = results.stderr
            else:
                results = subprocess.run(["compare",
                    os.path.join(refOutDir, imageFileName),
                    os.path.join(testOutDir, imageFileName),
                    "-metric", "PSNR", "-format", "'%[fx:mean*100]'", "info:"],
                    stderr=subprocess.PIPE)
                out = results.stderr.decode('utf-8')
            status = 'OK'
            # some versions of compare are returning inf and some return zero for identical images so we check for both.
            if out != 'inf' and out != '0' and out != '1.#INF':
                status = '<strong>DIFERENT</strong>'
                diffCount += 1
                print("**  difference detected in {category}/{filename}, results:'{results}'".format(
                    category=category, filename=imageFileName, results=out))
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

