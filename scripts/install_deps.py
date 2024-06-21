#!/usr/bin/env python3

import os, sys, argparse, traceback
from lib import env, cmd_utils

env.ensure_in_venv(__file__)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--pause-on-exit", action="store_true", help="Useful on Windows"
    )
    parser.add_argument(
        "--only", type=str, help="Only install the specified dependency"
    )
    args = parser.parse_args()

    try:
        deps = Dependencies(args.only)
        deps.install()
    except Exception:
        traceback.print_exc()

    if args.pause_on_exit:
        input("Press enter to continue...")


class Dependencies:

    def __init__(self, only):
        from lib import config

        self.config = config.Config()
        self.only = only
        self.ci_env = env.is_running_in_ci()

        if self.ci_env:
            print("CI environment detected")

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
        from lib import windows

        if not windows.is_admin():
            windows.relaunch_as_admin(__file__)
            sys.exit()

        only_qt = self.only == "qt"

        # for ci, skip qt; we install qt separately so we can cache it.
        if not self.ci_env or only_qt:
            qt = windows.WindowsQt(self.config.get_qt_config())
            qt_install_dir = qt.get_install_dir()
            if qt_install_dir:
                print(f"Skipping Qt, already installed at: {qt_install_dir}")
            else:
                qt.install()

            if not self.ci_env:
                qt.set_env_vars()

            if only_qt:
                return

        choco = windows.WindowsChoco()
        if self.ci_env:
            choco.config_ci_cache()
            choco_config_file, remove_packages = choco.get_config_file()
            choco.remove_from_config(choco_config_file, remove_packages)

        command = self.config.get_deps_command()
        choco.install(command, self.ci_env)

    def mac(self):
        """Installs dependencies on macOS."""
        from lib import mac

        command = self.config.get_os_deps_value("command")
        cmd_utils.run(command)

        if not self.ci_env:
            mac.set_cmake_prefix_env_var(self.config.get_os_value("qt-prefix-command"))

    def linux(self):
        """Installs dependencies on Linux."""

        distro = env.get_linux_distro()
        if not distro:
            raise RuntimeError("Unable to detect Linux distro")

        command = self.config.get_linux_deps_command(distro)

        has_sudo = cmd_utils.has_command("sudo")
        if "sudo" in command and not has_sudo:
            # assume we're running as root if sudo is not found (common on older distros).
            # a space char is intentionally added after "sudo" for intentionality.
            # possible limitation with stripping "sudo" is that if any packages with "sudo" in the
            # name are added to the list (probably very unlikely), this will have undefined behavior.
            print("The 'sudo' command was not found, stripping sudo from command")
            command = command.replace("sudo ", "").strip()

        cmd_utils.run(command)


if __name__ == "__main__":
    main()
