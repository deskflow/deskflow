import subprocess
import sys


def run(command, check=True, shell=True, get_stdout=False):
    """Runs a shell command.

    Args:
    command (str or list): The command to run.
    check (bool): If True, raises an exception if the command returns a non-zero exit code.
    get_stdout (bool): If True, returns the stdout of the command instead of forwarding it.
    """

    # To spread strings over multiple lines in YAML files, like in bash, a backslash is used at
    # the end of each line as continuation character.
    # When a YAML file is parsed, this becomes "\ ", so this character sequence must be removed
    # before running the command.
    # This doesn't seem to be an issue on Windows, since the \ path separator is rarely followed
    # by a space.
    cmd_continuation = "\\ "

    if isinstance(command, list):
        command = [c.replace(cmd_continuation, "") for c in command]
        command_str = " ".join(command)
    else:
        command = command.replace(cmd_continuation, "")
        command_str = command

    print(f"Running: {command_str}")
    sys.stdout.flush()

    try:
        if get_stdout:
            return subprocess.run(
                command,
                shell=shell,
                check=check,
                stdout=subprocess.PIPE,
                text=True,
            )
        else:
            return subprocess.run(command, check=check, shell=shell)
    except subprocess.CalledProcessError as e:
        print(f"Command failed: {command_str}", file=sys.stderr)
        raise e
