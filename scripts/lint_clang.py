#!/usr/bin/env python3

import argparse, sys
import lib.fs as fs
import lib.cmd_utils as cmd_utils

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
        "--format", action="store_true", help="Formats all files with CLang"
    )
    args = parser.parse_args()

    args = [] if args.format else ["--dry-run", "--Werror"]
    files_recursive = fs.find_files(dirs, include_files)

    if files_recursive:
        cmd_utils.run(["clang-format"] + args + files_recursive, print_cmd=True)
    else:
        print("No files for CLang to process.", file=sys.stderr)
        sys.exit(0)


if __name__ == "__main__":
    main()
