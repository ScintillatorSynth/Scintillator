#!/usr/bin/python3

import difflib
import os
import re
import subprocess
import sys

# code lifted from supercollider/tools/clang-format.py, then edited to suit
CLANG_FORMAT_ACCEPTED_VERSION_REGEX = re.compile("8\\.\\d+\\.\\d+")
CLANG_FORMAT_ACCEPTED_VERSION_STRING = "8.y.z"

def callo(args):
    """Call a program, and capture its output
    """
    return subprocess.check_output(args).decode('utf-8')

class ClangFormat(object):
    """Class encapsulates finding a suitable copy of clang-format,
    and linting/formating an individual file
    """
    def __init__(self, cf_cmd):
        self.cf_cmd = cf_cmd
#        if which(cf_cmd) is None:
#            raise ValueError("Could not find clang-format at %s" % cf_cmd)
        self._validate_version()

    def _validate_version(self):
        cf_version = callo([self.cf_cmd, "--version"])

        if CLANG_FORMAT_ACCEPTED_VERSION_REGEX.search(cf_version):
            return

        # TODO add instructions to check docs when docs are written
        raise ValueError("clang-format found, but incorrect version at " +
                self.cf_cmd + " with version: " + cf_version + "\nAccepted versions: " +
                CLANG_FORMAT_ACCEPTED_VERSION_STRING)
        sys.exit(5)

    def lint(self, file_name, print_diff):
        """Check the specified file has the correct format
        """
        with open(file_name, 'rb') as original_text:
            original_file = original_text.read().decode('utf-8')

        # Get formatted file as clang-format would format the file
        formatted_file = callo([self.cf_cmd, '-style=file', file_name])

        if original_file != formatted_file:
            if print_diff:
                original_lines = original_file.splitlines()
                formatted_lines = formatted_file.splitlines()
                result = difflib.unified_diff(original_lines, formatted_lines, file_name, file_name)
                for line in result:
                    print(line.rstrip())

            return False

        return True

    def format(self, file_name):
        """Update the format of the specified file
        """
        if self.lint(file_name, print_diff=False):
            return True

        # Update the file with clang-format
        formatted = not subprocess.call([self.cf_cmd, '-style=file', '-i', file_name])

        # Version 3.8 generates files like foo.cpp~RF83372177.TMP when it formats foo.cpp
        # on Windows, we must clean these up
        if sys.platform == "win32":
            glob_pattern = file_name + "*.TMP"
            for fglob in glob.glob(glob_pattern):
                os.unlink(fglob)

        return formatted


def main(argv):
    if len(argv) != 3 or (argv[0] != 'lint' and argv[0] != 'format'):
        print('clang-format.py <lint or format> <clang-format binary path> <Scintillator src/ dir>')
        sys.exit(2)

    cf = ClangFormat(argv[1])
    lint_ok = True
    linting = argv[0] == 'lint'

    for root, dirs, files in os.walk(argv[2]):
        for filename in files:
            base, ext = os.path.splitext(filename)
            if ext not in ['.cpp', '.hpp']:
                continue
            filename = os.path.join(root, filename)
            if linting:
                if not cf.lint(filename, True):
                    lint_ok = False
            else:
                if not cf.format(filename):
                    lint_ok = False

    if not lint_ok:
        sys.exit(2)

if __name__ == "__main__":
    main(sys.argv[1:])

