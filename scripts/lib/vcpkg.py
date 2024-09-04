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

import os
import git  # type: ignore
import lib.env as env
import lib.cmd_utils as cmd_utils

GIT_REPO = "https://github.com/microsoft/vcpkg.git"


def install():
    vcpkg_bin = ensure_vcpkg()

    cmd_utils.run([vcpkg_bin, "install"], print_cmd=True)


def ensure_vcpkg():
    if not os.path.exists("vcpkg"):
        get_vcpkg()
    else:
        print("Updating vcpkg...")
        repo = git.Repo("vcpkg")
        repo.remotes.origin.pull()

    if env.is_windows():
        vcpkg_bin = "vcpkg/vcpkg.exe"
    else:
        vcpkg_bin = "vcpkg/vcpkg"

    if not os.path.exists(vcpkg_bin):
        raise RuntimeError(f"Path not found: {vcpkg_bin}")

    return vcpkg_bin


def get_vcpkg():
    print("Downloading vcpkg...")
    repo = git.Repo.clone_from(GIT_REPO, "vcpkg")

    os.chdir("vcpkg")
    try:
        if env.is_windows():
            cmd_utils.run("bootstrap-vcpkg.bat", shell=True, print_cmd=True)
        else:
            cmd_utils.run("./bootstrap-vcpkg.sh", shell=True, print_cmd=True)
    finally:
        os.chdir("..")
