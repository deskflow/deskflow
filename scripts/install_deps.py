#!/usr/bin/env python3

from lib import env

import os
from lib import windows, mac, cmd_utils
import sys
import argparse
import traceback

if env.get_os() == "mac":
    # on mac, run in venv to make installing dependencies easier.
    env.ensure_in_venv("build/install_deps", __file__)

env.ensure_module("yaml", "pyyaml")
import yaml

config_file = "deps.yml"
required_packages = {
    "yaml": "pyyaml",
    "dotenv": "python-dotenv",
}


class YamlError(Exception):
    pass


class PlatformError(Exception):
    pass


class PathError(Exception):
    pass


def main():
    """Entry point for the script."""

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


class Config:
    """Reads the dependencies configuration file."""

    def __init__(self):
        with open(config_file, "r") as f:
            data = yaml.safe_load(f)

        self.os_name = env.get_os()
        try:
            root = data["dependencies"]
        except KeyError:
            raise YamlError(f"Nothing found in {config_file} for: dependencies")

        try:
            self.os = root[self.os_name]
        except KeyError:
            raise YamlError(f"Nothing found in {config_file} for: {self.os_name}")

    def get_qt_config(self):
        return self.get("qt")

    def get_linux_command(self, distro):
        distro_data = self.get(distro)
        try:
            return distro_data["command"]
        except KeyError:
            raise YamlError(f"No package command found in {config_file} for: {distro}")

    def get(self, key):
        try:
            return self.os[key]
        except KeyError:
            raise YamlError(f"Nothing found in {config_file} for: {self.os_name}:{key}")


class Dependencies:

    def __init__(self, only):
        self.config = Config()
        self.only = only
        self.ci_env = os.environ.get("CI")

        if self.ci_env:
            print("CI environment detected")

    def install(self):
        """Installs dependencies for the current platform."""

        os = env.get_os()
        if os == "windows":
            self.windows()
        elif os == "mac":
            self.mac()
        elif os == "linux":
            self.linux()
        else:
            raise PlatformError(f"Unsupported platform: {os}")

    def windows(self):
        """Installs dependencies on Windows."""

        if not windows.is_admin():
            windows.relaunch_as_admin(__file__)
            sys.exit()

        only_qt = self.only == "qt"

        # for ci, skip qt; we install qt separately so we can cache it.
        if not self.ci_env or only_qt:
            qt = windows.WindowsQt(self.config.get_qt_config(), config_file)
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

            try:
                ci_skip = self.config.get("ci")["skip"]
                choco_config_file = ci_skip["edit-config"]
                remove_packages = ci_skip["packages"]
            except KeyError:
                raise YamlError(f"Bad mapping in {config_file} on Windows for: ci")

            choco.remove_from_config(choco_config_file, remove_packages)

        command = self.config.get("command")
        choco.install(command, self.ci_env)

    def mac(self):
        """Installs dependencies on macOS."""
        command = self.config.get("command")
        cmd_utils.run(command)

        if not self.ci_env:
            mac.set_cmake_prefix_env_var(self.config.get("cmake-prefix"))

    def linux(self):
        """Installs dependencies on Linux."""

        distro = env.get_linux_distro()
        if not distro:
            raise PlatformError("Unable to detect Linux distro")

        command = self.config.get_linux_command(distro)
        cmd_utils.run(command)


if __name__ == "__main__":
    main()
