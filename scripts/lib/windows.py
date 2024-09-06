# Synergy -- mouse and keyboard sharing utility
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

import ctypes, sys, os, shutil
import xml.etree.ElementTree as ET
import lib.cmd_utils as cmd_utils
import lib.env as env
from lib.certificate import Certificate

LOCK_FILE = "tmp/elevated.lock"
MSBUILD_CMD = "msbuild"
SIGNTOOL_CMD = "signtool"
CERTUTIL_CMD = "certutil"
RUNNER_TEMP_ENV = "RUNNER_TEMP"
DIST_DIR = "dist"
BUILD_DIR = "build"
WIX_FILE = f"{BUILD_DIR}/installer/Synergy.sln"
MSI_FILE = f"{BUILD_DIR}/installer/bin/Release/Synergy.msi"


def run_elevated(script, args=None, use_sys_argv=True, waitForExit=False):
    if not args and use_sys_argv:
        args = " ".join(sys.argv[1:])

    if waitForExit:
        args += f" --lock-file {LOCK_FILE}"
        env.persist_lock_file(LOCK_FILE)

    command = f"{script} {args} --pause-on-exit"
    print(f"Running script with elevated privileges: {command}")

    WINDOW_HANDLE = None
    OPERATION = "runas"
    DIRECTORY = None
    SHOW_CMD = 1
    instance = ctypes.windll.shell32.ShellExecuteW(
        WINDOW_HANDLE, OPERATION, sys.executable, command, DIRECTORY, SHOW_CMD
    )

    ERROR_ACCESS_DENIED = 5
    if instance == ERROR_ACCESS_DENIED:
        raise RuntimeError(
            f"Failed to run script with elevated privileges, access denied (code {instance})"
        )

    ERROR_MAX = 32
    if instance <= ERROR_MAX:
        raise RuntimeError(
            f"Failed to run script with elevated privileges, error code: {instance}"
        )

    print("Script is running with elevated privileges")

    if waitForExit:
        with open(LOCK_FILE, "r") as f:
            pid = f.read()

        print(f"Waiting for elevated process to exit: {pid}")
        while os.path.exists(LOCK_FILE):
            # Intentionally wait forever, since this code should not run where a developer
            # has no control, such as in a CI environment.
            pass


def is_admin():
    """Returns True if the current process has admin privileges."""
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except ctypes.WinError:
        return False


def set_env_var(name, value):
    """
    Sets or updates an environment variable. Appends the value if it doesn't already exist.

    Args:
    name (str): The name of the environment variable.
    value (str): The value of the environment variable.
    """

    current_value = os.getenv(name, "")

    if value not in current_value:
        new_value = f"{current_value}{os.pathsep}{value}" if current_value else value
        os.environ[name] = new_value
        print(f"Setting environment variable: {name}={value}")
        cmd_utils.run(["setx", name, new_value], check=True, shell=True, print_cmd=True)


def package(filename_base):
    cert_env_key = "WINDOWS_PFX_CERTIFICATE"
    cert_base64 = env.get_env(cert_env_key, required=False)
    if cert_base64:
        cert_password = env.get_env("WINDOWS_PFX_PASSWORD")
        sign_binaries(cert_base64, cert_password)

    build_msi(filename_base)

    if cert_base64:
        sign_msi(filename_base, cert_base64, cert_password)
    else:
        print(f"Skipped code signing, env var not set: {cert_env_key}")


def assert_vs_cmd(cmd):
    has_cmd = cmd_utils.has_command(cmd)
    if not has_cmd:
        raise RuntimeError(
            f"The '{cmd}' command was not found, "
            "re-run from 'Developer Command Prompt for VS'"
        )


def build_msi(filename_base):
    print("Building MSI installer...")
    configuration = "Release"
    platform = "x64"

    assert_vs_cmd(MSBUILD_CMD)
    cmd_utils.run(
        [
            MSBUILD_CMD,
            WIX_FILE,
            f"/p:Configuration={configuration}",
            f"/p:Platform={platform}",
        ],
        shell=True,
        print_cmd=True,
    )

    path = get_package_path(filename_base)
    print(f"Copying MSI installer to {DIST_DIR}")
    os.makedirs(DIST_DIR, exist_ok=True)
    shutil.copy(MSI_FILE, path)


def get_package_path(filename_base):
    return f"{DIST_DIR}/{filename_base}.msi"


def sign_binaries(cert_base64, cert_password):
    exe_pattern = f"{BUILD_DIR}/bin/*.exe"
    run_codesign(exe_pattern, cert_base64, cert_password)


def sign_msi(filename_base, cert_base64, cert_password):
    path = get_package_path(filename_base)
    run_codesign(path, cert_base64, cert_password)


def run_codesign(path, cert_base64, cert_password):
    time_server = "http://timestamp.digicert.com"
    hashing_algorithm = "SHA256"

    with Certificate(cert_base64, "pfx") as cert_path:
        print("Signing MSI installer...")
        assert_vs_cmd(SIGNTOOL_CMD)

        # WARNING: contains private key password, never print this command
        cmd_utils.run(
            [
                SIGNTOOL_CMD,
                "sign",
                "/f",
                cert_path,
                "/p",
                cert_password,
                "/t",
                time_server,
                "/fd",
                hashing_algorithm,
                path,
            ]
        )


class WindowsChoco:
    """Chocolatey for Windows."""

    def ensure_choco_installed(self):
        if cmd_utils.has_command("choco"):
            return

        if not cmd_utils.has_command("winget"):
            print(
                "The winget command was not found, please install Chocolatey manually",
                file=sys.stderr,
            )
            sys.exit(1)

        print("The choco command was not found, installing Chocolatey...")
        cmd_utils.run(
            "winget install chocolatey",
            check=False,
            shell=True,
            print_cmd=True,
        )

        if not cmd_utils.has_command("choco"):
            print(
                "The choco command was still not found, please re-run this script...",
                file=sys.stderr,
            )
            sys.exit(1)
