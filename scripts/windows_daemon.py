import os, sys, time, subprocess, argparse, glob
import lib.windows as windows
import lib.file_utils as file_utils
import psutil

DEFAULT_BIN_NAME = "synergyd"
DEFAULT_SOURCE_DIR = os.path.join("build", "temp", "bin")
DEFAULT_TARGET_DIR = os.path.join("build", "bin")
SERVICE_NOT_RUNNING_ERROR = 2
ERROR_ACCESS_VIOLATION = 0xC0000005


def main():
    """Entry point for the script."""

    parser = argparse.ArgumentParser()
    parser.add_argument("--pause-on-exit", action="store_true")
    parser.add_argument("--source-dir", default=DEFAULT_SOURCE_DIR)
    parser.add_argument("--target-dir", default=DEFAULT_TARGET_DIR)
    parser.add_argument("--bin-name", default=DEFAULT_BIN_NAME)
    args = parser.parse_args()

    if not windows.is_admin():
        windows.relaunch_as_admin(__file__)
        sys.exit()

    try:
        reinstall(
            args.source_dir,
            args.target_dir,
            args.bin_name,
        )
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)

    if args.pause_on_exit:
        input("Press enter to continue...")


def reinstall(source_dir, target_dir, bin_name):
    """Stops the running daemon service, copies files, and reinstalls."""

    print("Stopping daemon service")
    try:
        subprocess.run(["net", "stop", "synergy"], shell=True, check=True)
    except subprocess.CalledProcessError as e:
        if e.returncode == SERVICE_NOT_RUNNING_ERROR:
            print("Daemon service not running")
        else:
            raise e

    source_bin_path = f"{os.path.join(source_dir, bin_name)}.exe"

    # Wait for Windows to release the file handles after process termination.
    while is_any_process_running(target_dir):
        print("Waiting for file handles to release")
        time.sleep(1)

    options = file_utils.CopyOptions(ignore_errors=True, verbose=False)
    print(f"Copying files from {source_dir} to {target_dir}")
    file_utils.copy(f"{source_dir}/*", target_dir, options)

    print("Removing old daemon service")
    try:
        subprocess.run([source_bin_path, "/uninstall"], shell=True, check=True)
    except subprocess.CalledProcessError as e:
        check_access_violation(e.returncode, source_bin_path)
        if e.returncode != 0:
            print(
                f"Warning: Uninstall failed, return code: {e.returncode}",
                file=sys.stderr,
            )

    target_bin_path = os.path.join(target_dir, bin_name + ".exe")

    try:
        print("Installing daemon service")
        subprocess.run([target_bin_path, "/install"], shell=True, check=True)
    except subprocess.CalledProcessError as e:
        check_access_violation(e.returncode, target_bin_path)
        if e.returncode != 0:
            print(f"Warning: Install failed, return code: {e.returncode}")


def check_access_violation(return_code, bin_path):
    if return_code == ERROR_ACCESS_VIOLATION:
        print(
            f"Warning: Process crashed with memory access violation: {bin_path}",
            file=sys.stderr,
        )


def is_any_process_running(dir):
    """Check if there is any running process that contains the given directory."""

    print(f"Checking if any process is running in: {dir}")
    for proc in psutil.process_iter(attrs=["name", "exe"]):
        exe = proc.info["exe"]

        if not exe:
            continue

        try:
            if dir.lower() in exe.lower():
                print(f"Process found: {exe}")
                return True
        except (psutil.NoSuchProcess, psutil.AccessDenied):
            pass
    return False


main()
