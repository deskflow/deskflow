#!/usr/bin/env python3

import os, sys, argparse, traceback
import lib.env as env
import lib.cmd_utils as cmd_utils


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--pause-on-exit", action="store_true", help="Useful on Windows"
    )
    parser.add_argument(
        "--only", type=str, help="Only install the specified dependency"
    )
    args = parser.parse_args()

    env.ensure_dependencies()
    env.ensure_in_venv(__file__, auto_create=True)
    env.install_requirements()

    error = False
    try:
        deps = Dependencies(args.only)
        deps.install()
    except Exception:
        traceback.print_exc()
        error = True

    if args.pause_on_exit:
        input("Press enter to continue...")

    if error:
        sys.exit(1)


class Dependencies:

    def __init__(self, only):
        from lib.config import Config

        self.config = Config()
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
        import lib.windows as windows

        if not windows.is_admin():
            windows.relaunch_as_admin(__file__)
            sys.exit()

        only_qt = self.only == "qt"

        # for ci, skip qt; we install qt separately so we can cache it.
        if not self.ci_env or only_qt:
            qt = windows.WindowsQt(*self.config.get_qt_config())
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
            edit_config, skip_packages = self.config.get_choco_ci_config()
            choco.remove_from_config(edit_config, skip_packages)

        command = self.config.get_deps_command()
        choco.install(command, self.ci_env)

    def mac(self):
        """Installs dependencies on macOS."""
        import lib.mac as mac

        command = self.config.get_os_deps_value("command")
        cmd_utils.run(command, shell=True, print_cmd=True)

        if not self.ci_env:
            mac.set_cmake_prefix_env_var(self.config.get_os_value("qt-prefix-command"))

    def linux(self):
        """Installs dependencies on Linux."""

        distro, _distro_like, _distro_version = env.get_linux_distro()
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

        # On Fedora, dnf update returns code 100 when updates are available, but the last command
        # run should be dnf install, so the return code should always be 0.
        cmd_utils.run(command, shell=True, print_cmd=True)


if __name__ == "__main__":
    main()
