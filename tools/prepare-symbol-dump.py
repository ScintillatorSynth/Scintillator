#!/usr/bin/python3
# Dumps the symbol table from the scintillator binary, and prepares the output in a format that can be consumed by the
# minidump_stackwalk tool. This tool expects that scinserver symbols are in a subdirectory with the name of the build
# id extracted from the first line of the symbol table, so for example if this is the first line from the symbol table:
#
# MODULE Linux x86_64 02263018FDF504EA7559EAF0B71F24AB0 scinsynth
#
# The minidump_stackwalk tool will expect this symbol file will be available at:
#
# ./symbols/scinsynth/02263018FDF504EA7559EAF0B71F24AB0/scinsynth.sym
#
# This python program:
# a) uses platform-specific dump_sym program to extract the symbol file
# b) parses the first line to get the symbol id
# c) creates the dump_id subdirectory, copies the symbol file into it
# d) creates a zipfile from the ./symbols/scinsynth directory so that when extracted the dump_id folder will be the root
# e) Name of the output zipfile is symbols-scinsynth.zip, will be moved and uploaded by Travis for ingestion by the
#    crash reporting system.

import sys
import os
import subprocess
import time
import select

def main(argv):
    # TODO: write me
    sys.exit(0)

if __name__ == "__main__":
    main(sys.argv[1:])

