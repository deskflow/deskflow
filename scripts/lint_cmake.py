#!/usr/bin/env python3

import sys, argparse
import lib.env as env
import lib.fs as fs

include_files = [
    "*.cmake",
    "CMakeLists.txt",
]

exclude_dirs = [
    "ext",
    "build",
]


def main():
    """
    Cross-platform equivalent of using find and xargs with cmake-format.
    Lints by performing a dry run (--check) which fails when formatting is needed.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("--format", action="store_true", help="Formats all CMake files")
    args = parser.parse_args()

    env.ensure_in_venv(__file__)
    from cmakelang.format.__main__ import main as cmake_format_main

    new_args = ["--in-place"] if args.format else ["--check"]
    files_recursive = fs.find_files(".", include_files, exclude_dirs)

    if files_recursive:
        sys.argv = [""] + new_args + files_recursive
        result = cmake_format_main()
        if result:
            print("CMake lint passed")
        else:
            sys.exit(1)
    else:
        print("No CMake files found to process.", file=sys.stderr)
        sys.exit(0)


if __name__ == "__main__":
    main()
