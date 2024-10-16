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

import os, sys
import lib.cmd_utils as cmd_utils
import lib.env as env

build_dir = "build/meson"


def meson_venv_bin():
    if not env.in_venv():
        raise RuntimeError("Not in a virtual environment")

    return os.path.join(os.path.dirname(sys.executable), "meson")


def setup(no_system_list, static_list):
    cmd = [meson_venv_bin(), "setup", build_dir]

    # TODO: These special Windows exceptions should probably be in Meson
    # or somewhere other than this script, as it's a bit hacky.
    if env.is_windows():
        cmd.append("-Dsystem-tomlplusplus=false")
        cmd.append("-Dsystem-cli11=false")

    for subproject in no_system_list or []:
        cmd.append(f"-Dsystem-{subproject}=false")

    # This might be a bit rude, but Meson seems to cache a lot (like CMake),
    # so wiping every time is the easiest way to ensure that the build is clean.
    # Plus, the way we're using Meson (at the moment) is just for satisfying
    # dependencies, so this script is run infrequently enough to not matter.
    if os.path.exists(build_dir):
        cmd.append("--wipe")

    cmd_utils.run(cmd, print_cmd=True)

    for subproject in static_list or []:
        static_subproject(subproject)


def static_subproject(subproject):
    if subproject == "libportal":
        # HACK: This is a bit horrible. Ideally, Meson would take care of applying this patch,
        # but it only seems to copy the .diff over and not apply the patch.
        #
        # Important: Static linking is not intended for package maintainers, only for beta testers.
        # Static linking is also pretty horrible, but many distros will be slow to pick up 0.8.x
        # which has input capture support. The sooner we can remove this patching code the better.
        cmd_utils.run(
            [
                "patch",
                "-d",
                "subprojects/libportal",
                "-p1",
                "-i",
                "static-lib.diff",
            ],
            print_cmd=True,
            check=False,
        )
    else:
        raise RuntimeError(f"Unknown subproject: {subproject}")


def compile():
    cmd_utils.run([meson_venv_bin(), "compile", "-C", build_dir], print_cmd=True)


def install():
    cmd = [meson_venv_bin(), "install", "-C", build_dir]

    has_sudo = cmd_utils.has_command("sudo")
    if has_sudo:
        cmd.insert(0, "sudo")

    cmd_utils.run(cmd, print_cmd=True)
