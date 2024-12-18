#!/usr/bin/env python3

# Deskflow -- mouse and keyboard sharing utility
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

import argparse, sys
import lib.fs as fs
from clang_format import clang_format  # type: ignore

include_files = [
    "*.h",
    "*.c",
    "*.hpp",
    "*.cpp",
    "*.m",
    "*.mm",
]

dirs = ["src"]


def main():
    """
    Cross-platform equivalent of using find and xargs with clang-format.
    Lints by performing a dry run (--dry-run) which fails when formatting is needed.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-f",
        "--format",
        action="store_true",
        help="In-place format all files",
    )
    args = parser.parse_args()

    cmd_args = ["-i"] if args.format else ["--dry-run", "--Werror"]
    files_recursive = fs.find_files(dirs, include_files)

    if args.format:
        print("Formatting files with Clang formatter:")
    else:
        print("Checking files with Clang formatter:")

    for file in files_recursive:
        print(file)

    if files_recursive:
        sys.argv = [""] + cmd_args + files_recursive
        result = clang_format()
        if result == 0:
            print("Clang lint passed")

        sys.exit(result)
    else:
        print("No files for Clang to process", file=sys.stderr)
        sys.exit(0)


if __name__ == "__main__":
    main()
