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

import argparse
import platform
from lib.linux import PackageType
from dotenv import load_dotenv  # type: ignore

env_file = ".env"
default_package_prefix = "synergy"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--leave-test-installed",
        action="store_true",
        help="Leave test package installed",
    )
    args = parser.parse_args()

    load_dotenv(dotenv_path=env_file)

    version = env.get_app_version()
    filename_base = get_filename_base(version)
    print(f"Package filename base: {filename_base}")

    if env.is_windows():
        windows_package(filename_base)
    elif env.is_mac():
        mac_package(filename_base)
    elif env.is_linux():
        linux_package(filename_base, version, args.leave_test_installed)
    else:
        raise RuntimeError(f"Unsupported platform: {env.get_os()}")


def get_filename_base(version, use_linux_distro=True):
    os = env.get_os()
    machine = platform.machine().lower()
    package_base = env.get_env("SYNERGY_PACKAGE_PREFIX", default=default_package_prefix)
    os_part = os

    if os == "linux" and use_linux_distro:
        distro_name, _distro_like, distro_version = env.get_linux_distro()
        if not distro_name:
            raise RuntimeError("Failed to detect Linux distro")

        if distro_version:
            version_for_filename = distro_version.replace(".", "-")
            os_part = f"{distro_name}-{version_for_filename}"
        else:
            os_part = distro_name

        # For consistency with existing filenames, we'll use 'amd64' instead of 'x86_64'.
        # Also, that's what Linux distros tend to call that architecture anyway.
        if machine == "x86_64":
            machine = "amd64"
    else:
        # Some Windows users get confused by 'amd64' and think it's 'arm64',
        # so we'll use Intel's 'x64' branding (even though it's wrong).
        # Also replace 'x86_64' with 'x64' for consistency.
        if machine == "amd64" or machine == "x86_64":
            machine = "x64"

    # Underscore is used to delimit different parts of the filename (e.g. version, OS, etc).
    # Dashes are used to delimit spaces, e.g. "debian-trixie" for "Debian Trixie".
    return f"{package_base}_{version}_{os_part}_{machine}"


def windows_package(filename_base):
    import lib.windows as windows

    windows.package(filename_base)


def mac_package(filename_base):
    import lib.mac as mac

    mac.package(filename_base)


def linux_package(filename_base, version, leave_test_installed):
    import lib.linux as linux

    extra_packages = env.get_env_bool("LINUX_EXTRA_PACKAGES", False)

    linux.package(filename_base, PackageType.DISTRO, leave_test_installed)

    if extra_packages:
        filename_base = get_filename_base(version, use_linux_distro=False)
        linux.package(filename_base, PackageType.TGZ)


if __name__ == "__main__":
    main()
