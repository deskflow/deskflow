#!/usr/bin/env python3

import os, sys, argparse, traceback
import lib.env as env
import lib.cmd_utils as cmd_utils
import lib.qt_utils as qt_utils
import lib.github as github

path_env_var = "PATH"
cmake_prefix_env_var = "CMAKE_PREFIX_PATH"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--pause-on-exit", action="store_true", help="Useful on Windows"
    )
    parser.add_argument(
        "--only-python",
        action="store_true",
        help="Only install Python dependencies",
    )
    parser.add_argument(
        "--ci-env",
        action="store_true",
        help="Set if running in CI environment",
    )
    args = parser.parse_args()

    env.ensure_dependencies()
    env.ensure_in_venv(__file__, auto_create=True)
    env.install_requirements()

    error = False
    if not args.only_python:
        try:
            deps = Dependencies(args.ci_env)
            deps.install()
        except Exception:
            traceback.print_exc()
            error = True

    if args.pause_on_exit:
        input("Press enter to continue...")

    if error:
        sys.exit(1)


class Dependencies:

    def __init__(self, ci_env):
        from lib.config import Config

        self.config = Config()
        self.ci_env = ci_env

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

        qt = qt_utils.WindowsQt(*self.config.get_qt_config())
        qt.install()

        if self.ci_env:
            github.set_env_var(cmake_prefix_env_var, qt.get_install_dir())
        else:
            windows.set_env_var(cmake_prefix_env_var, qt.get_install_dir())

        choco = windows.WindowsChoco()
        if self.ci_env:
            choco.config_ci_cache()
            edit_config, skip_packages = self.config.get_windows_ci_config()
            choco.remove_from_config(edit_config, skip_packages)

        command = self.config.get_deps_command()
        choco.install(command, self.ci_env)

    def mac(self):
        """Installs dependencies on macOS."""
        import lib.mac as mac

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

        command = self.config.get_os_deps_value("command")
        cmd_utils.run(command, shell=True, print_cmd=True)

        if env_vars_set:
            print(f"To load env vars, run: source {mac.shell_rc}")

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
