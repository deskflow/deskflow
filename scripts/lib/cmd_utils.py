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

import subprocess
import sys
import lib.env as env

try:
    import colorama  # type: ignore
    from colorama import Fore  # type: ignore

    colorama.init()
except ImportError:

    class Fore:
        RESET = ""
        YELLOW = ""


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


def strip_continuation_sequences(command, strip_newlines=True):
    """
    Remove the continuation sequences (\\) from a command.

    To spread strings over multiple lines in YAML files, like in bash, a backslash is used at
    the end of each line as continuation character.
    """

    if isinstance(command, list):
        raise ValueError("List commands are not supported")

    cmd_continuation = " \\"
    command = command.replace(cmd_continuation, "")

    # Some versions of pyyaml will remove the newlines already, so always stripping
    # makes the output more consistent.
    if strip_newlines:
        command = command.replace("\n", " ")

    return command


def run(
    command,
    check=True,  # true by default to fail fast
    shell=False,  # false by default for security
    get_output=False,
    print_cmd=False,  # false by default for security
):
    """
    Convenience wrapper around `subprocess.run` to:
    - print the command before running it (if `print_cmd` is True)

    This differs to `subprocess.run` in that by default it:
    - checks the return code by default
    - prints list commands as a readable string on failure

    This is the same as `subprocess.run` in that it:
    - does not use shell by default for security (shell is less secure)

    Args:
        command (str or list): The command to run.
        check (bool): Raise an exception if the command fails.
        shell (bool): Run the command in a shell (false by default for security)
        get_output (bool): Return the output of the command.
        print_cmd (bool): Print the command before running it (false by default for security)
    """

    is_list_cmd = isinstance(command, list)

    # create string version of list command, only for debugging purposes
    command_str = command
    if is_list_cmd:
        command_str = " ".join(command)

    if print_cmd:
        print(f"Running: {command_str}")
    else:
        print("Running command...")
        command_str = "***"

    # TODO: You can definitely use a list command with shell=True on Windows,
    # but can you use a string command with shell=False on Windows?
    #
    # The `subprocess.run` function has a little gotcha:
    # - a string command must be used when `shell=True`
    # - a list command must be used when shell isn't or `shell=False`
    # however, it allows you to pass a string command when shell isn't used or `shell=False`
    # then fails with a vague error message. same problem with list commands and `shell=True`
    if not env.is_windows() and is_list_cmd and shell:
        raise ValueError("List commands cannot be used when shell=True on Unix systems")
    elif not is_list_cmd and not shell:
        raise ValueError("String commands cannot be used when shell=False or not set")

    # Flush the output to ensure the command is printed before the output of the command,
    # which seems to happen in the GitHub runner logs.
    sys.stdout.flush()
    sys.stderr.flush()

    try:
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

    except subprocess.CalledProcessError as e:
        # Take control of how failed commands are printed:
        # - if `print_cmd` is false, it will print `***` instead of the command
        # - if the command was a list, the command is printed as a readable string
        raise RuntimeError(
            f"Command exited with code {e.returncode}: {command_str}"
        ) from None
    except Exception:
        # Take control of how failed commands are printed:
        # - if `print_cmd` is false, it will print `***` instead of the command
        # - if the command was a list, the command is printed as a readable string
        raise RuntimeError(f"Command failed: {command_str}")

    if result.returncode != 0:
        print(
            f"{Fore.YELLOW}Command exited with code {result.returncode}:{Fore.RESET} {command_str}",
            file=sys.stderr,
        )

    return result
