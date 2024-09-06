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

import os, sys, argparse, traceback
import lib.env as env
import lib.cmd_utils as cmd_utils
import lib.qt_utils as qt_utils
import lib.github as github
import lib.meson as meson

path_env_var = "PATH"
cmake_prefix_env_var = "CMAKE_PREFIX_PATH"


def main():
    is_ci = os.getenv("CI") is not None
    if is_ci:
        print("CI environment detected")

    args = parse_args(is_ci)
    if args.lock_file:
        env.persist_lock_file(args.lock_file)

    try:
        run(args)
    except Exception:
        traceback.print_exc()
        sys.exit(1)
    finally:
        if env.is_windows() and args.pause_on_exit:
            # Allow the rest of the install to continue while sitting at the pause.
            if args.lock_file:
                env.remove_lock_file(args.lock_file)

            # Useful on Windows, when elevated, Python is opened in a new window and closes
            # immediately after the script finishes. This keeps the script window open so that
            # the user can see the output.
            print()
            input("Press enter to continue...")


def parse_args(is_ci):
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--ci-env",
        action="store_true",
        help="Useful for faking CI env (defaults to true in CI env)",
        default=is_ci,
    )
    parser.add_argument(
        "--lock-file",
        type=str,
        help="Create a file to indicate script is running",
    )
    parser.add_argument(
        "--only-python", action="store_true", help="Only install Python dependencies"
    )
    parser.add_argument(
        "--skip-python",
        action="store_true",
        help="Do not install Python dependencies",
    )
    parser.add_argument(
        "--skip-system",
        action="store_true",
        help="Do not install system dependencies (apt, dnf, etc)",
    )
    parser.add_argument(
        "--skip-meson", action="store_true", help="Do not setup and compile with Meson"
    )
    parser.add_argument(
        "--subproject", type=str, help="Sub-project to install dependencies for"
    )
    parser.add_argument(
        "--meson-install",
        action="store_true",
        help="Install built Meson subprojects to system",
    )
    parser.add_argument(
        "--meson-no-system",
        nargs="+",
        help="Specify which Meson subprojects to use instead of system dependencies",
    )

    if env.is_windows():
        parser.add_argument(
            "--skip-vcpkg",
            action="store_true",
            help="Windows only: Do not install vcpkg dependencies",
        )
        parser.add_argument(
            "--skip-elevated",
            action="store_true",
            help="Windows only: Do not run elevated command",
        )
        parser.add_argument(
            "--only-elevated",
            action="store_true",
            help="Windows only: Only run elevated command",
        )
        parser.add_argument(
            "--pause-on-exit",
            action="store_true",
            help="Windows only: Useful to prevent elevated window from closing",
        )

    return parser.parse_args()


def run(args):
    env.ensure_dependencies()
    env.ensure_in_venv(__file__, auto_create=True)

    if not args.skip_python:
        env.install_requirements()

    colors = env.import_colors()

    if args.only_python:
        print()
        print(colors.SUCCESS_TEXT + " Only Python dependencies installed")
        return

    try:
        install(args)

        print()
        print(f"\n{colors.SUCCESS_TEXT} Dependencies installed")

        # On Windows and macOS, we set env vars for cmake, but for them to be picked up,
        # either the shell needs to be restarted or the env vars need to be re-sourced.
        # Restarting the shell is easier for most people.
        if not env.is_linux():
            print(
                f"{colors.WARNING_TEXT} You may need to restart your terminal "
                "or IDE to use new env vars"
            )
    except Exception:
        traceback.print_exc()
        print()
        print(f"\n{colors.ERROR_TEXT} Failed to install dependencies")
        sys.exit(1)


def install(args):
    if not args.skip_system:
        deps = Dependencies(args)
        deps.install()

    if args.subproject:
        deps = SubprojectDependencies(args.subproject)
        deps.install()
        return

    # Only install vcpkg dependencies on Windows, since on other OS it's not needed (yet).
    # We probably won't ever need this on macOS and Linux since brew and apt/dnf/etc do a
    # good job of providing dependencies. Where they don't, we can use Meson.
    if env.is_windows() and not args.skip_vcpkg:
        import lib.vcpkg as vcpkg

        vcpkg.install()

    if not args.skip_meson:
        run_meson(args.meson_install, args.meson_no_system)


# It's a bit weird to use Meson just for installing deps, but it's a stopgap until
# we fully switch from CMake to Meson. For the meantime, Meson will install the deps
# so that CMake can find them easily. Once we switch to Meson, it might be possible for
# Meson handle the deps resolution, so that we won't need to install them on the system.
def run_meson(install, no_system_list):
    meson.setup(no_system_list)

    # Only compile and install on Linux for now, since we're only using Meson to fetch
    # the deps on Windows and macOS.
    if env.is_linux():
        meson.compile()

    if install:
        meson.install()


class Dependencies:

    def __init__(self, args):
        from lib.config import Config

        self.config = Config()
        self.args = args
        self.ci_env = args.ci_env

    def install(self):
        """Installs dependencies for the current platform."""

        if env.is_windows():
            self.windows()
        elif env.is_mac():
            self.mac()
        elif env.is_linux():
            self.linux()
        else:
            raise RuntimeError(f"Unsupported platform: {os}")

    def windows(self):
        """Installs dependencies on Windows."""
        import lib.windows as windows

        if not self.args.skip_elevated:
            if not windows.is_admin():
                windows.run_elevated(__file__, "--only-elevated --skip-python")
            elif self.args.only_elevated:
                # The choco command should run from the elevated command.
                choco = windows.WindowsChoco()
                choco.ensure_choco_installed()
                command_elevated = self.config.get_os_deps_command("command-elevated")
                cmd_utils.run(command_elevated, shell=True, print_cmd=True)
                sys.exit(0)

        qt = qt_utils.WindowsQt(*self.config.get_qt_config())
        qt.install()

        if self.ci_env:
            github.set_env_var(cmake_prefix_env_var, qt.get_install_dir())
        else:
            windows.set_env_var(cmake_prefix_env_var, qt.get_install_dir())

        command = self.config.get_os_deps_command()

        cmd_utils.run(command, shell=True, print_cmd=True)

    def mac(self):
        """Installs dependencies on macOS."""
        import lib.mac as mac

        # On macOS, brew does have a Qt package available, but it is always built against the
        # current macOS version and the brew version also does some really weird stuff with the
        # library symbols, which confuses the heck out of `macqtdeploy`. So, using the official
        # Qt library binaries seems to be the most reliable option for distribution.
        qt = qt_utils.MacQt(*self.config.get_qt_config())
        qt.install()

        qt_dir = qt.get_install_dir()
        qt_bin_dir = os.path.join(qt_dir, "bin")
        env_vars_set = 0
        if self.ci_env:
            github.set_env_var(cmake_prefix_env_var, qt_dir)
            github.add_to_path(qt_bin_dir)
        else:
            env_vars_set += mac.set_env_var(cmake_prefix_env_var, qt_dir)
            env_vars_set += mac.set_env_var(path_env_var, qt_bin_dir)

        command = self.config.get_os_deps_command()
        cmd_utils.run(command, shell=True, print_cmd=True)

        if env_vars_set:
            print(f"To load env vars, run: source {mac.shell_rc}")

    def linux(self):
        """Installs dependencies on Linux."""
        import lib.linux as linux

        distro, distro_like, _distro_version = env.get_linux_distro()
        if not distro:
            raise RuntimeError("Unable to detect Linux distro")

        command_pre = self.config.get_os_deps_command_pre(
            linux_distro=distro, required=False
        )
        if command_pre:
            print("Running dependencies prerequisites command")

            check = True
            if distro == "fedora" or (distro_like and "fedora" in distro_like):
                print(
                    "Fedora-like detected, "
                    "ignoring return code on dependencies prerequisites command"
                )
                # On Fedora-like, dnf update returns code 100 when updates are available.
                check = False

            linux.run_command(command_pre, check)

        command = self.config.get_os_deps_command(linux_distro=distro)
        optional = self.config.get_os_deps_value(
            "optional", linux_distro=distro, required=False
        )
        for optional_package in optional or []:
            if not linux.is_package_available(optional_package):
                print(f"Optional package not found, stripping: {optional_package}")
                command = command.replace(optional_package, "")

        linux.run_command(command, check=True)

        subprojects = self.config.get_os_subprojects()
        if subprojects:
            for subproject in subprojects:
                deps = SubprojectDependencies(subproject)
                deps.install()


class SubprojectDependencies:

    def __init__(self, subproject):
        from lib.config import Config

        self.subproject = subproject
        self.config = Config()

    def install(self):
        """Installs dependencies for the current platform."""

        print(f"Installing dependencies for sub-project: {self.subproject}")

        if env.is_linux():
            self.linux()
        else:
            raise RuntimeError(f"Unsupported platform: {os}")

    def linux(self):
        """Installs dependencies on Linux."""
        import lib.linux as linux

        command = self.config.get_subproject_deps_command(self.subproject)
        linux.run_command(command, check=True)


if __name__ == "__main__":
    main()
