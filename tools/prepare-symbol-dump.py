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
# a) uses dump_sym program to extract the symbol file
# b) parses the first line of the dump to get the symbol id
# c) creates the dump_id subdirectory, copies the symbol file into it

import os
import select
import shutil
import subprocess
import sys
import time

def main(argv):
    if not os.path.exists('Scintillator.quark'):
        print('Please run this script from the Scintillator Quark root directory.')
        sys.exit(1)

    print('extracting symbols from scinsynth')
    dump_syms = subprocess.run(['build/install-ext/bin/dump_syms', '-v', 'build/src/scinsynth'], stdout=subprocess.PIPE)
    # syms is typically ~10MB of text data
    syms = dump_syms.stdout.decode('utf-8')

    # extract first line from syms
    first_line = syms[0:syms.index('\n')].split(' ')
    os_name = first_line[1]
    dump_id = first_line[3]
    print('got dump_id ' + dump_id + ' on ' + os_name)

    sym_path = os.path.join('build', 'symbols', 'scinsynth', dump_id)

    # Feeling some paranoia that there might be other stuff in this directory so we delete it first
    if os.path.exists(os.path.join('build', 'symbols')):
        print('removing existing symbols path')
        shutil.rmtree(os.path.join('build', 'symbols'))
    os.makedirs(sym_path)

    # save symbol file
    print('saving symbol file to ' + os.path.join(sym_path, 'scinsynth.sym'))
    sym_file = open(os.path.join(sym_path, 'scinsynth.sym'), 'w')
    sym_file.write(syms)
    sym_file.close()

    sys.exit(0)

if __name__ == "__main__":
    main(sys.argv[1:])

