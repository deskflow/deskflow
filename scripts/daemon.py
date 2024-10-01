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

import os, sys, argparse
import lib.windows as windows
import lib.colors as colors

DEFAULT_SERVICE_ID = "deskflow"
DEFAULT_BIN_NAME = "deskflow-daemon"
DEFAULT_SOURCE_DIR = os.path.join("build", "temp", "bin")
DEFAULT_TARGET_DIR = os.path.join("build", "bin")
IGNORE_PROCESSES = ["deskflow.exe"]


def main():
    """Entry point for the script."""

    parser = argparse.ArgumentParser()
    parser.add_argument("--reinstall", action="store_true")
    parser.add_argument("--stop", action="store_true")
    parser.add_argument("--restart", action="store_true")
    parser.add_argument("--pause-on-exit", action="store_true")
    parser.add_argument("--source-dir", default=DEFAULT_SOURCE_DIR)
    parser.add_argument("--target-dir", default=DEFAULT_TARGET_DIR)
    parser.add_argument("--bin-name", default=DEFAULT_BIN_NAME)
    parser.add_argument("--ignore-processes", nargs="+", default=IGNORE_PROCESSES)
    parser.add_argument("--service-id", default=DEFAULT_SERVICE_ID)
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args()

    if not env.is_windows():
        print(
            f"{colors.ERROR_TEXT} This script is only supported on Windows",
            file=sys.stderr,
        )
        sys.exit(1)

    service = windows.WindowsService(__file__, args)

    try:
        if args.reinstall:
            service.reinstall()
        elif args.stop:
            service.stop()
        elif args.restart:
            service.restart()
        else:
            print("No action specified", file=sys.stderr)
            exit(1)
    except Exception as e:
        print(f"{colors.ERROR_TEXT} {e}", file=sys.stderr)

    if args.pause_on_exit:
        input("Press enter to continue...")


if __name__ == "__main__":
    main()
