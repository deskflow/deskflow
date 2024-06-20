import os
from lib import cmd_utils

cmake_env_var = "CMAKE_PREFIX_PATH"
shell_rc = "~/.zshrc"


def append_to_zshrc(text):
    file = os.path.expanduser(shell_rc)
    with open(file, "r") as f:
        if text in f.read():
            return

    with open(file, "a") as f:
        f.write(f"\n{text}")
        print(f"Appended to {shell_rc}: {text}")


def add_cmake_prefix(cmake_prefix_command):
    output = cmd_utils.run(cmake_prefix_command, get_stdout=True)
    cmake_prefix = output.strip()

    print(f"Setting environment variable: {cmake_env_var}={cmake_prefix}")
    append_to_zshrc(f'export {cmake_env_var}="${cmake_env_var}:{cmake_prefix}"')
