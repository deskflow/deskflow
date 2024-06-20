import subprocess
import sys
from contextlib import redirect_stdout
import io


def run(command, check=True, get_stdout=False):
    """Runs a shell command.

    Args:
    command (str or list): The command to run.
    check (bool): If True, raises an exception if the command returns a non-zero exit code.
    get_stdout (bool): If True, returns the stdout of the command instead of forwarding it.
    """

    command_str = command
    if isinstance(command, list):
        command_str = " ".join(command)

    print(f"Running: {command_str}")
    sys.stdout.flush()

    try:
        if get_stdout:
            result = subprocess.run(
                command,
                shell=True,
                check=check,
                stdout=subprocess.PIPE,
                text=True,
            )
            return result.stdout
        else:
            subprocess.run(command, check=check, shell=True)
    except subprocess.CalledProcessError as e:
        print(f"Command failed: {command_str}", file=sys.stderr)
        raise e
