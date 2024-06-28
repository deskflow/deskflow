#!/usr/bin/env python3

import sys, os, argparse, fnmatch
import lib.env as env

include_files = [
    "*.cmake",
    "CMakeLists.txt",
]

exclude_dirs = [
    "ext",
    "build",
]


def find_files(base_dir, include_files, exclude_dirs):
    """Recursively find files, excluding specified directories"""
    matches = []
    for root, dirnames, filenames in os.walk(base_dir):
        # Exclude specified directories
        dirnames[:] = [d for d in dirnames if d not in exclude_dirs]

        for pattern in include_files:
            for filename in fnmatch.filter(filenames, pattern):
                matches.append(os.path.join(root, filename))
    return matches


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--format", action="store_true", help="Formats all CMake files")
    args = parser.parse_args()

    env.ensure_in_venv(__file__)

    from cmakelang.format.__main__ import main as cmake_format_main

    new_args = ["--in-place"] if args.format else ["--check"]
    files_recursive = find_files(".", include_files, exclude_dirs)

    if files_recursive:
        sys.argv = [""] + new_args + files_recursive
        sys.exit(cmake_format_main())
    else:
        print("No CMake files found to process.")
        sys.exit(0)


if __name__ == "__main__":
    main()
