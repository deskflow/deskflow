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

import os, sys, subprocess
import lib.cmd_utils as cmd_utils

# The `.venv` dir seems to be most common for virtual environments.
VENV_DIR = ".venv"


def check_module(module):
    try:
        __import__(module)
        return True
    except ImportError:
        print(f"Python is missing {module} module", file=sys.stderr)
        return False


def get_os():
    """Detects the operating system."""
    if sys.platform == "win32":
        return "windows"
    elif sys.platform == "darwin":
        return "mac"
    elif sys.platform.startswith("linux"):
        return "linux"
    else:
        raise RuntimeError(f"Unsupported platform: {sys.platform}")


def is_windows():
    return get_os() == "windows"


def is_mac():
    return get_os() == "mac"


def is_linux():
    return get_os() == "linux"


def get_linux_distro():
    """Detects the Linux distro."""
    os_file = "/etc/os-release"
    name = None
    name_like = None
    version_id = None
    version_codename = None

    if os.path.isfile(os_file):
        with open(os_file) as f:
            for line in f:
                if line.startswith("ID="):
                    name = line.strip().split("=")[1].strip('"')
                elif line.startswith("ID_LIKE="):
                    name_like = line.strip().split("=")[1].strip('"')
                elif line.startswith("VERSION_ID="):
                    version_id = line.strip().split("=")[1].strip('"')
                elif line.startswith("VERSION_CODENAME="):
                    version_codename = line.strip().split("=")[1].strip('"')

    return name, name_like, version_id or version_codename


def get_env(name, required=True, default=None):
    """
    Returns an env var (stripped) or optionally raises an error if not set.

    If `default` is set, it will be returned even if `required` is True.
    """
    value = os.getenv(name)
    if value:
        value = value.strip()

    if not value:
        if default:
            return default
        elif required:
            raise ValueError(f"Required env var not set: {name}")

    return value


def get_env_bool(name, default=False):
    """Returns a boolean value from an env var (stripped)."""
    value = os.getenv(name)
    if value:
        value = value.strip()

    if value is None:
        return default

    return value.lower() in ["true", "1", "yes"]


def get_venv_executable(binary="python"):
    if sys.platform == "win32":
        return os.path.join(VENV_DIR, "Scripts", binary)
    else:
        return os.path.join(VENV_DIR, "bin", binary)


def in_venv():
    """Returns True if the script is running in a Python virtual environment."""
    return sys.prefix != sys.base_prefix


def ensure_in_venv(script_file, create_venv=False):
    """
    Ensures the script is running in a Python virtual environment (venv).
    If the script is not running in a venv, it will create one and re-run the script in the venv.
    """

    check_dependencies(raise_error=True)
    import venv

    if in_venv():
        print(f"Running in venv, executable: {sys.executable}", flush=True)
        return

    if create_venv and not os.path.exists(VENV_DIR):
        print(f"Creating virtual environment at {VENV_DIR}")
        venv.create(VENV_DIR, with_pip=True)

    if os.path.exists(VENV_DIR):
        script_file_abs = os.path.abspath(script_file)
        print(f"Using virtual environment for: {script_file_abs}", flush=True)
        python_executable = get_venv_executable()
        result = subprocess.run([python_executable, script_file_abs] + sys.argv[1:])
        sys.exit(result.returncode)
    else:
        print(
            "The Python virtual environment (.venv) needs to be created before you can "
            "run this script.\n"
            "Please run: scripts/setup_venv.py"
        )
        sys.exit(1)


def install_requirements():
    """
    Uses `pip` to install required Python modules from the `requirements.txt` file.
    """

    check_dependencies(raise_error=True)

    print("Updating pip...")
    cmd_utils.run(
        [sys.executable, "-m", "pip", "install", "--upgrade", "pip"],
        shell=False,
        print_cmd=True,
    )

    print("Installing required modules...")
    cmd_utils.run(
        [sys.executable, "-m", "pip", "install", "-e", "scripts"],
        shell=False,
        print_cmd=True,
    )


def check_dependencies(raise_error=False):
    """
    Returns True if pip and venv are available.
    """

    has_pip = check_module("pip")
    has_venv = check_module("venv")

    if raise_error:
        if not has_pip:
            raise RuntimeError("Python is missing pip")
        if not has_venv:
            raise RuntimeError("Python is missing venv")
    else:
        return has_pip and has_venv


def ensure_dependencies():
    """
    Ensures that pip and venv are available, and installs them if they are not.
    This is normally only installs on Linux, as Windows and Mac usually come with pip and venv.
    """

    if check_dependencies():
        return

    print("Installing Python dependencies...")

    os = get_os()
    if os != "linux":
        # should not be a problem, since windows and mac come with pip and venv
        raise RuntimeError(f"Unable to install Python dependencies on {os}")

    has_sudo = cmd_utils.has_command("sudo")
    sudo = "sudo" if has_sudo else ""

    distro, distro_like, _version = get_linux_distro()
    if not distro_like:
        distro_like = distro

    update_cmd = None
    install_cmd = None
    if distro == "rhel" or "rhel" in distro_like:
        update_cmd = "yum check-update"
        install_cmd = "yum install -y python3-pip"  # rhel-like has venv already
    elif "debian" in distro_like:
        update_cmd = "apt update"
        install_cmd = "apt install -y python3-pip python3-venv"
    elif "fedora" in distro_like:
        update_cmd = "dnf check-update"
        install_cmd = "dnf install -y python3-pip python3-virtualenv"
    elif "arch" in distro_like:
        install_cmd = "pacman -Syu --noconfirm python-pip python-virtualenv"
    elif "opensuse" in distro_like:
        update_cmd = "zypper refresh"
        install_cmd = "zypper install -y python3-pip python3-virtualenv"
    else:
        raise RuntimeError(f"Unable to install Python dependencies on {distro}")

    if update_cmd:
        # don't check the return code, as some package managers return non-zero exit codes
        # under normal circumstances (e.g. dnf check-update returns 100 when there are
        # updates available).
        cmd_utils.run(
            f"{sudo} {update_cmd}".strip(), check=False, shell=True, print_cmd=True
        )

    cmd_utils.run(f"{sudo} {install_cmd}".strip(), shell=True, print_cmd=True)


def import_colors():
    import lib.colors as colors

    return colors


def persist_lock_file(path):
    """
    Persists a lock file and ensures the directory part of the path exists.
    """
    dir_path = os.path.dirname(path)
    if not os.path.exists(dir_path):
        os.makedirs(dir_path, exist_ok=True)

    with open(path, "w") as f:
        f.write(str(os.getpid()))


def remove_lock_file(path):
    """
    Removes a lock file if it exists.
    """
    if os.path.exists(path):
        os.remove(path)
