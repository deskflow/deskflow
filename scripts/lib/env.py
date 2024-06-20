import os
import sys
from lib import cmd_utils


def check_module(module):
    try:
        __import__(module)
        return True
    except ImportError:
        print(f"Python is missing {module} module")
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


def get_linux_distro():
    """Detects the Linux distro."""
    os_file = "/etc/os-release"
    if os.path.isfile(os_file):
        with open(os_file) as f:
            for line in f:
                if line.startswith("ID="):
                    return line.strip().split("=")[1].strip('"')
    return None


def get_python_executable(venv_path):
    if sys.platform == "win32":
        return os.path.join(venv_path, "Scripts", "python")
    else:
        return os.path.join(venv_path, "bin", "python")


def in_venv():
    """Returns True if the script is running in a Python virtual environment."""
    return sys.prefix != sys.base_prefix


def ensure_in_venv(venv_path, script):
    """
    Ensures the script is running in a Python virtual environment (venv).
    If the script is not running in a venv, it will create one and re-run the script in the venv.
    """

    ensure_dependencies()
    import venv

    if not in_venv():
        if not os.path.exists(venv_path):
            print(f"Creating virtual environment at {venv_path}")
            venv.create(venv_path, with_pip=True)

        script_file = os.path.basename(script)
        print(f"Re-running {script_file} in virtual environment...")
        python_executable = get_python_executable(venv_path)
        result = cmd_utils.run([python_executable, script] + sys.argv[1:], shell=False)
        sys.exit(result.returncode)
    else:
        print("Now running in virtual environment")


def ensure_module(module, package):
    """
    Ensures that a Python module is available, and installs the package if it is not.
    """

    ensure_dependencies()

    try:
        __import__(module)
    except ImportError:
        print(f"Python is missing {module} module, installing {package} package...")
        cmd_utils.run([sys.executable, "-m", "pip", "install", package], shell=False)
    else:
        print(f"Python package {package} already installed")


def ensure_dependencies():
    """
    Ensures that pip and venv are available, and installs them if they are not.
    This is normally only required on Linux.
    """

    has_pip = check_module("pip")
    has_venv = check_module("venv")

    if has_pip and has_venv:
        return

    print("Installing Python dependencies...")

    os = get_os()
    if os != "linux":
        # should not be a problem, since windows and mac come with pip and venv
        raise RuntimeError(f"Unable to install Python dependencies on {os}")

    has_sudo = cmd_utils.has_command("sudo")
    sudo = "sudo" if has_sudo else ""

    distro = get_linux_distro()
    if distro == "ubuntu" or distro == "debian":
        cmd_utils.run(f"{sudo} apt update".trim(), check=False)
        cmd_utils.run(f"{sudo} apt install -y python3-pip python3-venv".trim())
    elif distro == "fedora" or distro == "centos":
        cmd_utils.run(f"{sudo} dnf check-update".trim(), check=False)
        cmd_utils.run(f"{sudo} dnf install -y python3-pip python3-virtualenv".trim())
    else:
        # arch, opensuse, etc, patches welcome! :)
        raise RuntimeError(f"Unable to install Python dependencies on {distro}")
