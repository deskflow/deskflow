import subprocess
import sys


def run(command, check=True):
    """Runs a shell command and by default asserts that the return code is 0."""

    command_str = command
    if isinstance(command, list):
        command_str = " ".join(command)

    print(f"Running: {command_str}")
    sys.stdout.flush()

    try:
        subprocess.run(command, shell=True, check=check)
    except subprocess.CalledProcessError as e:
        print(f"Command failed: {command_str}", file=sys.stderr)
        raise e
