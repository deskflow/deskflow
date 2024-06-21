import os
import subprocess
import sys
import argparse
import glob
from lib import windows

BIN_NAME = "synergyd"
SOURCE_BIN_DIR = os.path.join("build", "bin")
TARGET_BIN_DIR = "bin"
SERVICE_NOT_RUNNING_ERROR = 2


def main():
    """Entry point for the script."""

    parser = argparse.ArgumentParser()
    parser.add_argument("--pause-on-exit", action="store_true")
    parser.add_argument("--source-bin-dir", default=SOURCE_BIN_DIR)
    parser.add_argument("--target-bin-dir", default=TARGET_BIN_DIR)
    parser.add_argument("--source-bin-name", default=BIN_NAME)
    parser.add_argument("--target-bin-name", default=BIN_NAME)
    args = parser.parse_args()

    if not windows.is_admin():
        windows.relaunch_as_admin(__file__)
        sys.exit()

    try:
        reinstall(
            args.source_bin_dir,
            args.target_bin_dir,
            args.source_bin_name,
            args.target_bin_name,
        )
    except Exception as e:
        print(f"Error: {e}")

    if args.pause_on_exit:
        input("Press enter to continue...")


def reinstall(source_bin_dir, target_bin_dir, source_bin_name, target_bin_name):
    """Stops the running daemon service, copies files, and reinstalls."""

    print("Stopping daemon service")
    try:
        subprocess.run(["net", "stop", "synergy"], shell=True, check=True)
    except subprocess.CalledProcessError as e:
        if e.returncode == SERVICE_NOT_RUNNING_ERROR:
            print("Daemon service not running")
        else:
            raise e

    copy_bin_files(source_bin_dir, target_bin_dir, source_bin_name, target_bin_name)

    target_bin_file = f"{os.path.join(target_bin_dir, target_bin_name)}.exe"

    print("Removing old daemon service")
    subprocess.run([target_bin_file, "/uninstall"], shell=True, check=True)

    print("Installing daemon service")
    subprocess.run([target_bin_file, "/install"], shell=True, check=True)


def copy_bin_files(source_bin_dir, target_bin_dir, source_bin_name, target_bin_name):

    if not os.path.isdir(source_bin_dir):
        raise RuntimeError(f"Invalid source bin dir: {source_bin_dir}")

    print(f"Persisting dir: {target_bin_dir}")
    os.makedirs(target_bin_dir, exist_ok=True)

    source_bin_glob = f"{source_bin_name}*"
    source_files = glob.glob(os.path.join(source_bin_dir, source_bin_glob))

    if not source_files:
        raise RuntimeError(
            f"No files found in {source_bin_dir} matching {source_bin_glob}"
        )

    for source_file in source_files:
        base_name = os.path.basename(source_file)
        base_name = base_name.replace(source_bin_name, target_bin_name)
        target_file = os.path.join(target_bin_dir, base_name)
        print(f"Copying {source_file} to {target_file}")
        # use the copy command; shutil.copy gives us a permission denied error.
        try:
            subprocess.run(["copy", source_file, target_file], shell=True, check=True)
        except subprocess.CalledProcessError as e:
            print(f"Copy failed: {e}")


main()
