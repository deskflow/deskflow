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


def strip_continuation_sequences(command):
    """
    Remove the continuation sequences (\\) from a command.

    To spread strings over multiple lines in YAML files, like in bash, a backslash is used at
    the end of each line as continuation character.
    When a YAML file is parsed, this becomes "\\ " (without a new line char), so this character
    sequence must be removed before running the command.
    This doesn't seem to be an issue on Windows, since the \\ path separator is rarely followed
    by a space.
    """
    cmd_continuation = "\\ "

    if isinstance(command, list):
        return [c.replace(cmd_continuation, "") for c in command]
    else:
        return command.replace(cmd_continuation, "")


# TODO: fix bug: often when using this function, only the first arg element is sent to subprocess.run
def run(
    command,
    check=True,
    shell=True,
    get_output=False,
    print_cmd=True,
):
    """
    Convenience wrapper around `subprocess.run` to:
    - print the command before running it
    - pipe/capture the output instead of printing it

    This differs to `subprocess.run` in that by default it:
    - checks the return code by default
    - uses a shell by default (sometimes a bad idea for security)

    Warning: This code is used by CI and prints the command before running it;
    never use this function with sensitive information such as passwords,
    unless you want the world to know.

    Args:
        command (str or list): The command to run.
        check (bool): Raise an exception if the command fails.
        shell (bool): Run the command in a shell.
        get_output (bool): Return the output of the command.
        print_cmd (bool): Print the command before running it.
    """

    # create string version of list command, only for debugging purposes
    command_str = command
    if isinstance(command, list):
        command_str = " ".join(command)

    if print_cmd:
        print(f"Running: {command_str}")
        sys.stdout.flush()

    if get_output:
        result = subprocess.run(
            command,
            shell=shell,
            check=check,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
    else:
        result = subprocess.run(command, check=check, shell=shell)

    if print_cmd and result.returncode != 0:
        print(
            f"Command exited with code {result.returncode}: {command_str}",
            file=sys.stderr,
        )
    return result
