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

import ctypes, sys, os, shutil, time, subprocess
import lib.cmd_utils as cmd_utils
import lib.env as env
import psutil  # type: ignore
import lib.colors as colors
import lib.file_utils as file_utils

LOCK_FILE = "tmp/elevated.lock"
RUNNER_TEMP_ENV = "RUNNER_TEMP"
SERVICE_NOT_RUNNING_ERROR = 2
ERROR_ACCESS_VIOLATION = 0xC0000005


def run_elevated(script, args=None, use_sys_argv=True, wait_for_exit=False):
    if not args and use_sys_argv:
        args = " ".join(sys.argv[1:])

    if wait_for_exit:
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

    if wait_for_exit:
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

def assert_vs_cmd(cmd):
    has_cmd = cmd_utils.has_command(cmd)
    if not has_cmd:
        raise RuntimeError(
            f"The '{cmd}' command was not found, "
            "re-run from 'Developer Command Prompt for VS'"
        )

class WindowsService:
    def __init__(self, script, args):
        self.script = script
        self.verbose = args.verbose
        self.bin_name = args.bin_name
        self.source_dir = os.path.abspath(args.source_dir)
        self.target_dir = os.path.abspath(args.target_dir)
        self.service_id = args.service_id
        self.ignore_processes = args.ignore_processes

    def print_verbose(self, message):
        if self.verbose:
            print(message)

    def ensure_admin(self):
        if not is_admin():
            run_elevated(self.script)
            sys.exit()

    def restart(self):
        """Stops the daemon service, copies files, and restarts the daemon service."""

        self.ensure_admin()
        self.stop()
        self.copy_files()
        self.start()

    def reinstall(self):
        """Stops and uninstalls daemon service, copies files, and reinstalls the daemon service."""

        self.ensure_admin()
        self.stop()

        source_bin_path = f"{os.path.join(self.source_dir, self.bin_name)}.exe"

        self.copy_files()

        print("Removing old daemon service")
        try:
            subprocess.run([source_bin_path, "/uninstall"], shell=True, check=True)
        except subprocess.CalledProcessError as e:
            self.check_access_violation(e.returncode, source_bin_path)
            if e.returncode != 0:
                print(
                    f"{colors.WARNING_TEXT} Uninstall failed, return code: {e.returncode}",
                    file=sys.stderr,
                )

        target_bin_path = os.path.join(self.target_dir, self.bin_name + ".exe")

        try:
            print("Installing daemon service")
            subprocess.run([target_bin_path, "/install"], shell=True, check=True)
        except subprocess.CalledProcessError as e:
            self.check_access_violation(e.returncode, target_bin_path)
            if e.returncode != 0:
                print(
                    f"{colors.WARNING_TEXT} Install failed, return code: {e.returncode}"
                )

    def copy_files(self):
        options = file_utils.CopyOptions(ignore_errors=True, verbose=False)
        print(f"Copying files from {self.source_dir} to {self.target_dir}")
        file_utils.copy(f"{self.source_dir}/*", self.target_dir, options)

    def stop(self):
        self.ensure_admin()
        print("Stopping daemon service")
        try:
            subprocess.run(["net", "stop", self.service_id], shell=True, check=True)
        except subprocess.CalledProcessError as e:
            if e.returncode == SERVICE_NOT_RUNNING_ERROR:
                self.print_verbose("Daemon service not running")
            else:
                raise e

        # Wait for Windows to release the file handles after process termination.
        self.wait_for_stop()

    def start(self):
        self.ensure_admin()
        print("Starting daemon service")
        subprocess.run(["net", "start", self.service_id], shell=True, check=True)

    def wait_for_stop(self):
        if self.is_any_process_running(self.target_dir):
            print("Waiting for file handles to release...", end="", flush=True)
            while self.is_any_process_running(self.target_dir):
                if not self.verbose:
                    print(".", end="", flush=True)
                time.sleep(1)
            if not self.verbose:
                print()

    def check_access_violation(self, return_code, bin_path):
        if return_code == ERROR_ACCESS_VIOLATION:
            print(
                f"{colors.WARNING_TEXT} Process crashed with memory access violation: {bin_path}",
                file=sys.stderr,
            )

    def is_ignored_process(self, exe):
        for ignore_process in self.ignore_processes:
            if exe.endswith(ignore_process):
                return True
        return False

    def is_any_process_running(self, dir):
        """Check if there is any running process that contains the given directory."""

        self.print_verbose(f"Checking if any process is running in: {dir}")

        for proc in psutil.process_iter(attrs=["name", "exe"]):
            exe = proc.info["exe"]

            if not exe:
                self.print_verbose(f"Skipping process with no exe: {proc}")
                continue

            if self.is_ignored_process(exe):
                self.print_verbose(f"Ignoring process: {exe}")
                continue

            try:
                if dir.lower() in exe.lower():
                    self.print_verbose(f"Process found: {exe}")
                    return True
            except (psutil.NoSuchProcess, psutil.AccessDenied):
                pass

        return False
