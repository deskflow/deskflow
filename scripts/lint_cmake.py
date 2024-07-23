#!/usr/bin/env python3

import lib.env as env

env.ensure_in_venv(__file__)

import sys, argparse
import lib.fs as fs
from cmakelang.format.__main__ import main as cmake_format_main  # type: ignore

include_files = [
    "*.cmake",
    "CMakeLists.txt",
]

exclude_dirs = ["ext", "build", "deps"]


def main():
    """
    Cross-platform equivalent of using find and xargs with cmake-format.
    Lints by performing a dry run (--check) which fails when formatting is needed.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--format",
        action="store_true",
        help="In-place format all files",
    )
    args = parser.parse_args()

    cmd_args = ["--in-place"] if args.format else ["--check"]
    files_recursive = fs.find_files(".", include_files, exclude_dirs)

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
