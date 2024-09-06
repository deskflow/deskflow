#!/usr/bin/env python3

# Synergy -- mouse and keyboard sharing utility
# Copyright (C) 2024 Symless Ltd.
#
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file LICENSE that should have accompanied this file.
#
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import lib.env as env

env.ensure_in_venv(__file__)

import sys, argparse
import lib.fs as fs
from cmakelang.format.__main__ import main as cmake_format_main  # type: ignore

INCLUDE_FILES = [
    "*.cmake",
    "CMakeLists.txt",
]

EXCLUDE_DIRS = [
    "build",
    ".venv",
    "deps",
    "subprojects",
]


def main():
    """
    Cross-platform equivalent of using find and xargs with cmake-format.
    Lints by performing a dry run (--check) which fails when formatting is needed.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-f",
        "--format",
        action="store_true",
        help="In-place format all files",
    )
    args = parser.parse_args()

    cmd_args = ["--in-place"] if args.format else ["--check"]
    files_recursive = fs.find_files(".", INCLUDE_FILES, EXCLUDE_DIRS)

    if args.format:
        print("Formatting files with CMake formatter:")
    else:
        print("Checking files with CMake formatter:")

    for file in files_recursive:
        print(file)

    if files_recursive:
        sys.argv = [""] + cmd_args + files_recursive

        result = cmake_format_main()
        if result == 0:
            print("CMake lint passed")

        sys.exit(result)
    else:
        print("No CMake files found to process.", file=sys.stderr)
        sys.exit(0)


if __name__ == "__main__":
    main()
