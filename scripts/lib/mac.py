import os
from lib import cmd_utils

cmake_env_var = "CMAKE_PREFIX_PATH"
shell_rc = "~/.zshrc"


def set_env_var(name, value):
    text = f'export {name}="${name}:{value}"'
    file = os.path.expanduser(shell_rc)
    with open(file, "r") as f:
        if text in f.read():
            return

    print(f"Setting environment variable: {name}={name}")
    with open(file, "a") as f:
        f.write(f"\n{text}")
        print(f"Appended to {shell_rc}: {text}")


def set_cmake_prefix_env_var(cmake_prefix_command):
    result = cmd_utils.run(cmake_prefix_command, get_stdout=True)
    cmake_prefix = result.stdout.strip()

    set_env_var(cmake_env_var, cmake_prefix)
