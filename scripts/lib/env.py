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


def bootstrap():
    has_pip = check_module("pip")
    has_venv = check_module("venv")

    if not has_pip or not has_venv:
        install_dependencies()


def install_dependencies():
    print("Installing Python dependencies...")

    os = get_os()
    if os != "linux":
        raise RuntimeError(f"Failed to install Python dependencies on {os}")

    distro = get_linux_distro()
    if distro == "ubuntu" or distro == "debian":
        print("Ubuntu detected, installing python3-pip...")
        cmd_utils.run(
            ["sudo", "apt", "install", "-y", "python3-pip", "python3-venv"], shell=False
        )
    elif distro == "fedora" or distro == "centos":
        print("Fedora detected, installing python3-pip...")
        cmd_utils.run(
            ["sudo", "dnf", "install", "-y", "python3-pip", "python3-venv"], shell=False
        )
    else:
        raise RuntimeError(f"Failed to install Python dependencies on {distro}")


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

    bootstrap()
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

    bootstrap()

    try:
        __import__(module)
    except ImportError:
        print(f"Python is missing {module} module, installing {package} package...")
        cmd_utils.run([sys.executable, "-m", "pip", "install", package], shell=False)
    else:
        print(f"Python package {package} already installed")
