import subprocess
import sys


def has_command(command):
    platform = sys.platform
    if platform == "win32":
        cmd = f"where {command}"
    else:
        cmd = f"which {command}"
    try:
        subprocess.check_output(cmd, shell=True)
        return True
    except subprocess.CalledProcessError:
        return False


def run(command, check=True, shell=True, get_stdout=False):
    """
    Run a command. By default, shell is used and the return code is checked.

    Args:
        command (str or list): Command to run.
        check (bool): If True, raise an exception if the command fails.
        shell (bool): If True, run the command in a shell.
        get_stdout (bool): If True, return the output of the command.
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
            result = subprocess.run(
                command,
                shell=shell,
                check=check,
                stdout=subprocess.PIPE,
                text=True,
            )
        else:
            result = subprocess.run(command, check=check, shell=shell)
    except subprocess.CalledProcessError as e:
        print(
            f"Command failed with code {e.returncode}: {command_str}", file=sys.stderr
        )
        raise e

    if result.returncode != 0:
        print(
            f"Command failed with code {result.returncode}: {command_str}",
            file=sys.stderr,
        )
