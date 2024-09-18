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

import argparse
import lib.file_utils as file_utils
import lib.colors as colors


def main():
    """
    Cross platform script to copy files and directories.
    This script was mostly created beause the default `copy` command on Windows is too noisy.
    If this becomes complex it must be replaced with a library.
    """

    parser = argparse.ArgumentParser()
    parser.add_argument("source", help="Source pattern to copy from")
    parser.add_argument("target", help="Destination pattern to copy to")
    parser.add_argument(
        "--ignore-errors", action="store_true", help="Ignore errors when copying"
    )
    parser.add_argument(
        "--verbose", action="store_true", help="Print more information to the console"
    )
    args = parser.parse_args()

    options = file_utils.CopyOptions(args.ignore_errors, args.verbose)

    try:
        file_utils.copy(args.source, args.target, options)
    except Exception as e:
        if not args.ignore_errors:
            raise e
        else:
            print(f"{colors.ERROR_TEXT} {e}")


if __name__ == "__main__":
    main()
