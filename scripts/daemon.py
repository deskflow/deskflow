#!/usr/bin/env python3

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

import lib.env as env

env.ensure_in_venv(__file__)

import os, sys, time, subprocess, argparse
import lib.windows as windows
import psutil  # type: ignore
import lib.colors as colors
import lib.file_utils as file_utils

DEFAULT_SOURCE_BIN = "synergyd"
DEFAULT_TARGET_BIN = "synergyd"
DEFAULT_SOURCE_DIR = os.path.join("build", "temp", "bin")
DEFAULT_TARGET_DIR = os.path.join("build", "bin")
SERVICE_NOT_RUNNING_ERROR = 2
ERROR_ACCESS_VIOLATION = 0xC0000005
IGNORE_PROCESSES = ["synergy.exe"]


class Context:
    def __init__(self, verbose):
        self.verbose = verbose


def main():
    """Entry point for the script."""

    parser = argparse.ArgumentParser()
    parser.add_argument("--reinstall", action="store_true")
    parser.add_argument("--stop", action="store_true")
    parser.add_argument("--restart", action="store_true")
    parser.add_argument("--pause-on-exit", action="store_true")
    parser.add_argument("--source-dir", default=DEFAULT_SOURCE_DIR)
    parser.add_argument("--target-dir", default=DEFAULT_TARGET_DIR)
    parser.add_argument("--source-bin", default=DEFAULT_SOURCE_BIN)
    parser.add_argument("--target-bin", default=DEFAULT_TARGET_BIN)
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args()

    if not env.is_windows():
        print(
            f"{colors.ERROR_TEXT} This script is only supported on Windows",
            file=sys.stderr,
        )
        sys.exit(1)

    context = Context(args.verbose)

    try:
        if args.reinstall:
            reinstall(
                context,
                args.source_dir,
                args.target_dir,
                args.source_bin,
                args.target_bin,
            )
        elif args.stop:
            stop(context, args.target_dir)
        elif args.restart:
            restart(context, args.source_dir, args.target_dir)
        else:
            print("No action specified", file=sys.stderr)
            exit(1)
    except Exception as e:
        print(f"{colors.ERROR_TEXT} {e}", file=sys.stderr)

    if args.pause_on_exit:
        input("Press enter to continue...")


def print_verbose(context, message):
    if context.verbose:
        print(message)


def ensure_admin():
    if not windows.is_admin():
        windows.run_elevated(__file__)
        sys.exit()


def restart(context, source_dir, target_dir):
    """Stops the daemon service, copies files, and restarts the daemon service."""

    ensure_admin()
    stop(context, target_dir)
    copy_files(source_dir, target_dir)
    start()


def reinstall(context, source_dir, target_dir, source_bin, target_bin):
    """Stops and uninstalls daemon service, copies files, and reinstalls the daemon service."""

    ensure_admin()
    stop(context, target_dir)

    source_bin_path = f"{os.path.join(source_dir, source_bin)}.exe"

    copy_files(source_dir, target_dir)

    print("Removing old daemon service")
    try:
        subprocess.run([source_bin_path, "/uninstall"], shell=True, check=True)
    except subprocess.CalledProcessError as e:
        check_access_violation(e.returncode, source_bin_path)
        if e.returncode != 0:
            print(
                f"{colors.WARNING_TEXT} Uninstall failed, return code: {e.returncode}",
                file=sys.stderr,
            )

    target_bin_path = os.path.join(target_dir, target_bin + ".exe")

    try:
        print("Installing daemon service")
        subprocess.run([target_bin_path, "/install"], shell=True, check=True)
    except subprocess.CalledProcessError as e:
        check_access_violation(e.returncode, target_bin_path)
        if e.returncode != 0:
            print(f"{colors.WARNING_TEXT} Install failed, return code: {e.returncode}")


def copy_files(source_dir, target_dir):
    options = file_utils.CopyOptions(ignore_errors=True, verbose=False)
    print(f"Copying files from {source_dir} to {target_dir}")
    file_utils.copy(f"{source_dir}/*", target_dir, options)


def stop(context, target_dir):
    ensure_admin()
    print("Stopping daemon service")
    try:
        subprocess.run(["net", "stop", "synergy"], shell=True, check=True)
    except subprocess.CalledProcessError as e:
        if e.returncode == SERVICE_NOT_RUNNING_ERROR:
            print_verbose(context, "Daemon service not running")
        else:
            raise e

    # Wait for Windows to release the file handles after process termination.
    wait_for_stop(context, target_dir)


def start():
    ensure_admin()
    print("Starting daemon service")
    subprocess.run(["net", "start", "synergy"], shell=True, check=True)


def wait_for_stop(context, target_dir):
    if is_any_process_running(context, target_dir):
        print("Waiting for file handles to release...", end="", flush=True)
        while is_any_process_running(context, target_dir):
            if not context.verbose:
                print(".", end="", flush=True)
            time.sleep(1)
        if not context.verbose:
            print()


def check_access_violation(return_code, bin_path):
    if return_code == ERROR_ACCESS_VIOLATION:
        print(
            f"{colors.WARNING_TEXT} Process crashed with memory access violation: {bin_path}",
            file=sys.stderr,
        )


def is_ignored_process(exe):
    for ignore_process in IGNORE_PROCESSES:
        if exe.endswith(ignore_process):
            return True
    return False


def is_any_process_running(context, dir):
    """Check if there is any running process that contains the given directory."""

    print_verbose(context, f"Checking if any process is running in: {dir}")

    for proc in psutil.process_iter(attrs=["name", "exe"]):
        exe = proc.info["exe"]

        if not exe:
            print_verbose(context, f"Skipping process with no exe: {proc}")
            continue

        if is_ignored_process(exe):
            print_verbose(context, f"Ignoring process: {exe}")
            continue

        try:
            if dir.lower() in exe.lower():
                print_verbose(context, f"Process found: {exe}")
                return True
        except (psutil.NoSuchProcess, psutil.AccessDenied):
            pass

    return False


main()
