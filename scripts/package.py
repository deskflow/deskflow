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
import platform
from dotenv import load_dotenv  # type: ignore

ENV_FILE = ".env"
DEFAULT_PRODUCT_NAME = "Deskflow"
DEFAULT_FILENAME_BASE = "deskflow"
DEFAULT_PROJECT_BUILD_DIR = "build"
DEFAULT_DIST_DIR = "dist"
DEFAULT_PACKAGE_NAME = "deskflow"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--package-version",
        help="Set the Package Version",
        required=True)
    parser.add_argument(
        "--leave-test-installed",
        action="store_true",
        help="Leave test package installed",
    )
    args = parser.parse_args()

    load_dotenv(dotenv_path=ENV_FILE)

    package(
        DEFAULT_FILENAME_BASE,
        DEFAULT_PROJECT_BUILD_DIR,
        DEFAULT_DIST_DIR,
        DEFAULT_PRODUCT_NAME,
        version=args.package_version,
    )

def package(
    filename_prefix,
    project_build_dir,
    dist_dir,
    product_name,
    version,
    source_dir=None,
):
    filename_base = get_filename_base(version, filename_prefix)
    print(f"Package filename base: {filename_base}")

    if env.is_windows():
        windows_package(filename_base, project_build_dir, dist_dir)
    else:
        raise RuntimeError(f"Unsupported platform: {env.get_os()}")


def get_filename_base(version, prefix):
    os = env.get_os()
    machine = platform.machine().lower()
    os_part = os

    if os == "windows":
        # Some Windows users get confused by 'amd64' and think it's 'arm64',
        # so we'll use Intel's 'x64' branding (even though it's wrong).
        # Also replace 'x86_64' with 'x64' for consistency.
        os_part= "win"
        if machine == "amd64" or machine == "x86_64":
            machine = "x64"
    return f"{prefix}-{version}-{os_part}-{machine}"



def windows_package(filename_base, project_build_dir, dist_dir):
    import lib.windows as windows

    windows.package(filename_base, project_build_dir, dist_dir)

if __name__ == "__main__":
    main()
