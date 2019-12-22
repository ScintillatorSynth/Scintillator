#!/usr/bin/python3

import json
import os
import sys

def main(argv):
    if len(argv) != 4:
        print('filter-coverage-report.py <inputfile> <Scintillator src/ dir> <commithash> <outputfile>')
        sys.exit(2)
    total_lines = 0
    total_covered = 0
    # first parse the provided dictionary and extract only the src/ specific files (we don't track coverage of
    # stuff in third_party/)
    json_in_file = open(argv[0], 'r')
    json_in = json.load(json_in_file)
    json_in_file.close()
    file_counts = {};
    for file_dict in json_in['data'][0]['files']:
        filename = file_dict['filename']
        if filename.startswith(argv[1]):
            # no fair double-dipping with unit test code (which always has 100% coverage)
            if filename.endswith("_unittests.cpp"):
                continue
            file_counts[filename] = file_dict['summary']['lines']
            total_lines += file_dict['summary']['lines']['count']
            total_covered += file_dict['summary']['lines']['covered']

    # Now some files are completely absent from the report, so traverse the source tree to add all sources and line
    # counts for a complete coverage summary.
    for root, dirs, files in os.walk(argv[1]):
        for filename in files:
            base, ext = os.path.splitext(filename)
            # skip files that aren't source files
            if ext not in ['.cpp', '.hpp']:
                continue
            # also skip unittests here so they don't count as uncovered files.
            if filename.endswith("_unittests.cpp"):
                continue
            filename = os.path.join(root, filename)
            if filename not in file_counts:
                line_count = 0
                for line in open(filename, 'r'):
                    line_count += 1
                total_lines += line_count
                file_counts[filename] = { 'count': line_count, 'covered': 0, 'percent': 0 }

    total_percent = (float(total_covered) / float(total_lines)) * 100.0
    summary = { 'summary': { 'count': total_lines, 'covered': total_covered, 'percent': total_percent }}
    summary['files'] = file_counts
    summary['hash'] = argv[2]
    json_out_file = open(argv[3], 'w')
    json.dump(summary, json_out_file, sort_keys=True)
    json_out_file.close()
    print('total lines: %d, total covered: %d, %f percent' % (total_lines, total_covered, total_percent))

if __name__ == "__main__":
    main(sys.argv[1:])

