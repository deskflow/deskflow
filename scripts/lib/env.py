import os, sys, subprocess
import lib.cmd_utils as cmd_utils

venv_path = "build/python"


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
    version = None

    if os.path.isfile(os_file):
        with open(os_file) as f:
            for line in f:
                if line.startswith("ID="):
                    name = line.strip().split("=")[1].strip('"')
                elif line.startswith("ID_LIKE="):
                    name_like = line.strip().split("=")[1].strip('"')
                elif line.startswith("VERSION_ID="):
                    version = line.strip().split("=")[1].strip('"')

    return name, name_like, version


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


def get_python_executable(binary="python"):
    if sys.platform == "win32":
        return os.path.join(venv_path, "Scripts", binary)
    else:
        return os.path.join(venv_path, "bin", binary)


def in_venv():
    """Returns True if the script is running in a Python virtual environment."""
    return sys.prefix != sys.base_prefix


def ensure_in_venv(script_file, auto_create=False):
    """
    Ensures the script is running in a Python virtual environment (venv).
    If the script is not running in a venv, it will create one and re-run the script in the venv.
    """

    check_dependencies(raise_error=True)
    import venv

    if not in_venv():
        if not os.path.exists(venv_path):
            if not auto_create:
                print("Hint: Run the `install_deps.py` script first.")
                raise RuntimeError(f"Virtual environment not found at: {venv_path}")

            print(f"Creating virtual environment at {venv_path}")
            venv.create(venv_path, with_pip=True)

        script_file_abs = os.path.abspath(script_file)
        print(f"Using virtual environment for: {script_file_abs}", flush=True)
        python_executable = get_python_executable()
        result = subprocess.run([python_executable, script_file_abs] + sys.argv[1:])
        sys.exit(result.returncode)


def install_requirements():
    """
    Uses `pip` to install required Python modules from the `requirements.txt` file.
    """

    check_dependencies(raise_error=True)

    print("Installing required modules...")
    cmd_utils.run(
        [sys.executable, "-m", "pip", "install", "-r", "scripts/requirements.txt"],
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
    if "debian" in distro_like:
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


def get_app_version():
    """
    Returns the version either from the env var, or from the version file.
    """
    version = get_env("SYNERGY_VERSION", required=False)
    if version:
        return version

    with open("VERSION", "r") as f:
        return f.read().strip()
