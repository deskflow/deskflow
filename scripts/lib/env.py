import os, sys, subprocess, platform
from lib import env, cmd_utils

venv_path = "build/python"
version_env = "build/.env.version"


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


def is_running_in_ci():
    """Returns True if running in a CI environment."""
    return os.environ.get("CI")


def get_linux_distro():
    """Detects the Linux distro."""
    os_file = "/etc/os-release"
    if os.path.isfile(os_file):
        with open(os_file) as f:
            for line in f:
                if line.startswith("ID="):
                    return line.strip().split("=")[1].strip('"')
    return None


def get_env_var(name):
    """Returns an env var or raises an error if it is not set."""
    value = os.getenv(name)
    if not value:
        raise ValueError(f"Environment variable not set: {name}")
    return value


def get_python_executable(binary="python"):
    if sys.platform == "win32":
        return os.path.join(venv_path, "Scripts", binary)
    else:
        return os.path.join(venv_path, "bin", binary)


def in_venv():
    """Returns True if the script is running in a Python virtual environment."""
    return sys.prefix != sys.base_prefix


def ensure_in_venv(script):
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
        print(f"Using virtual environment for {script_file}")
        sys.stdout.flush()
        python_executable = get_python_executable()
        result = subprocess.run([python_executable, script] + sys.argv[1:])
        sys.exit(result.returncode)


# TODO: Use pyproject.toml to specify dependencies
def ensure_module(module, package):
    """
    Ensures that a Python module is available, and installs the package if it is not.
    """

    ensure_dependencies()

    try:
        __import__(module)
    except ImportError:
        print(f"Python missing {module}, installing {package}...", file=sys.stderr)
        cmd_utils.run([sys.executable, "-m", "pip", "install", package], shell=False)


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
        cmd_utils.run(f"{sudo} apt update".strip(), check=False)
        cmd_utils.run(f"{sudo} apt install -y python3-pip python3-venv".strip())
    elif distro == "fedora" or distro == "centos":
        cmd_utils.run(f"{sudo} dnf check-update".strip(), check=False)
        cmd_utils.run(f"{sudo} dnf install -y python3-pip python3-virtualenv".strip())
    else:
        # arch, opensuse, etc, patches welcome! :)
        raise RuntimeError(f"Unable to install Python dependencies on {distro}")


def get_version_info():
    env.ensure_module("dotenv", "python-dotenv")
    from dotenv import load_dotenv  # type: ignore

    if not os.path.isfile(version_env):
        raise RuntimeError(f"Version file not found: {version_env}")

    load_dotenv(dotenv_path=version_env)

    major = os.getenv("SYNERGY_VERSION_MAJOR")
    minor = os.getenv("SYNERGY_VERSION_MINOR")
    patch = os.getenv("SYNERGY_VERSION_PATCH")
    stage = os.getenv("SYNERGY_VERSION_STAGE")
    build = os.getenv("SYNERGY_VERSION_BUILD")

    return major, minor, patch, stage, build
