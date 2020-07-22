#!/usr/bin/python3
#
# Wrapper around the breakpad minidump_stackwalk binary that can also fetch symbols for scinsynth if they aren't present
# in the local symbol storage. Can execute on a provided dump file name or local crash database. Can provide machine and
# human-readable outputs.

import argparse
import os
import subprocess
import sys

def main(args):
    if not os.path.exists('Scintillator.quark'):
        print('Please run this script from the Scintillator Quark root directory.')
        sys.exit(1)

    if args.dump_file:
        dump_files = [ args.dump_file ]
    else:
        # Use the crashpad_database_util to enumerate the crash reports in the database and get their paths.
        database_reports = subprocess.run(['build/install-ext/crashpad/out/Default/crashpad_database_util',
            '--show-client-id', '--show-pending-reports', '--show-completed-reports', '--show-all-report-info', '-d',
            args.database], stdout=subprocess.PIPE).stdout.decode('utf-8')
        print(database_reports)
        # extract files with the Path: qualifier
        dump_files = [ x.strip().split(' ')[1]
                for x in database_reports.split('\n') if x.strip().split(' ')[0] == 'Path:' ]

    for p in dump_files:
        minidump_options = ['build/install-ext/bin/minidump_stackwalk']
        if args.m:
            minidump_options.append('-m')
        minidump_options.append(p)
        minidump_options.append(args.symbols)
        minidump_run = subprocess.run(minidump_options, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        print('---- minidump ' + p + ':')
        print(minidump_run.stdout.decode('utf-8'))

    if args.fail_with_nonempty_database:
        sys.exit(len(dump_files))

    sys.exit(0)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Helper script for breakpad minidump_stackwalk')
    parser.add_argument('-m', help="Use machine-readable format for stackwalk output", action='store_true')
    parser.add_argument('--symbols', help='Path to a directory symbols directory cache', default='build/symbols')
    parser.add_argument('--fail_with_nonempty_database',
            help='Exit with a non-zero exit code if a dump file was processed.', action='store_true')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--database', help='Path to a crash report database. Every crash report will be stackwalked')
    group.add_argument('--dump_file', help='Path to a single minidump file to stackwalk.')
    args = parser.parse_args()
    main(args)

