#!/usr/bin/env python3

import argparse, sys
import lib.fs as fs
import lib.env as env

include_files = [
    "*.h",
    "*.c",
    "*.hpp",
    "*.cpp",
]

dirs = ["src"]


def main():
    """
    Cross-platform equivalent of using find and xargs with clang-format.
    Lints by performing a dry run (--dry-run) which fails when formatting is needed.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--format",
        action="store_true",
        help="In-place format all files",
    )
    args = parser.parse_args()

    env.ensure_in_venv(__file__)
    from clang_format import clang_format  # type: ignore

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
