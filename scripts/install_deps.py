import os
from lib import windows, cmd_utils
import sys
import argparse
import traceback

config_file = "deps.yml"


class YamlError(Exception):
    pass


class PlatformError(Exception):
    pass


class PathError(Exception):
    pass


try:
    import yaml  # type: ignore
except ImportError:
    # this is fairly common in earlier versions of python3,
    # which is normally what you find on mac and windows.
    print("Python yaml module missing, please install: pip install pyyaml")
    sys.exit(1)


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


def get_os():
    """Detects the operating system."""
    if sys.platform == "win32":
        return "windows"
    elif sys.platform == "darwin":
        return "mac"
    elif sys.platform.startswith("linux"):
        return "linux"
    else:
        raise PlatformError(f"Unsupported platform: {sys.platform}")


def get_linux_distro():
    """Detects the Linux distro."""
    os_file = "/etc/os-release"
    if os.path.isfile(os_file):
        with open(os_file) as f:
            for line in f:
                if line.startswith("ID="):
                    return line.strip().split("=")[1].strip('"')
    return None


class Config:
    """Reads the dependencies configuration file."""

    def __init__(self):
        with open(config_file, "r") as f:
            data = yaml.safe_load(f)

        os_name = get_os()
        try:
            root = data["dependencies"]
        except KeyError:
            raise YamlError(f"Nothing found in {config_file} for: dependencies")

        try:
            self.os = root[os_name]
        except KeyError:
            raise YamlError(f"Nothing found in {config_file} for: {os_name}")

    def get_qt_config(self):
        try:
            return self.os["qt"]
        except KeyError:
            raise YamlError(f"Nothing found in {config_file} for: qt")

    def get_packages_file(self):
        try:
            return self.os["packages"]
        except KeyError:
            raise YamlError(f"Nothing found in {config_file} for: packages")

    def get_linux_package_command(self, distro):
        try:
            distro_data = self.os[distro]
        except KeyError:
            raise YamlError(f"Nothing found in {config_file} for: {distro}")

        try:
            command_base = distro_data["command"]
        except KeyError:
            raise YamlError(f"No package command found in {config_file} for: {distro}")

        try:
            package_data = distro_data["packages"]
        except KeyError:
            raise YamlError(f"No package list found in {config_file} for: {distro}")

        packages = " ".join(package_data)
        return f"{command_base} {packages}"


class Dependencies:

    def __init__(self, only):
        self.config = Config()
        self.only = only

    def install(self):
        """Installs dependencies for the current platform."""

        os = get_os()
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

        ci_env = os.environ.get("CI")
        if ci_env:
            print("CI environment detected")

        only_qt = self.only == "qt"

        # for ci, skip qt; we install qt separately so we can cache it.
        if not ci_env or only_qt:
            qt = windows.WindowsQt(self.config.get_qt_config(), config_file)
            qt_install_dir = qt.get_install_dir()
            if qt_install_dir:
                print(f"Skipping Qt, already installed at: {qt_install_dir}")
            else:
                qt.install()

            if only_qt:
                return

        choco = windows.WindowsChoco()
        if ci_env:
            choco.config_ci_cache()

            try:
                ci_skip = self.config.os["ci"]["skip"]
                choco_config_file = ci_skip["edit-config"]
                remove_packages = ci_skip["packages"]
            except KeyError:
                raise YamlError(f"Bad mapping in {config_file} on Windows for: ci")

            choco.remove_from_config(choco_config_file, remove_packages)

        try:
            command = self.config.os["command"]
        except KeyError:
            raise YamlError(f"Nothing found in {config_file} on Windows for: command")

        choco.install(command, ci_env)

    def mac(self):
        """Installs dependencies on macOS."""
        try:
            command = self.config.os["command"]
        except KeyError:
            raise YamlError(f"Nothing found in {config_file} on Mac for: command")

        cmd_utils.run(command)

    def linux(self):
        """Installs dependencies on Linux."""

        distro = get_linux_distro()
        if not distro:
            raise PlatformError("Unable to detect Linux distro")

        command = self.config.get_linux_package_command(distro)
        cmd_utils.run(command)


main()
