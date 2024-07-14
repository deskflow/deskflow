import os

github_env_key = "GITHUB_ENV"
github_path_key = "GITHUB_PATH"


def set_env_var(key, value):
    """
    Appends the key=value pair to the GitHub environment file.
    """
    env_file = os.getenv(github_env_key)
    if not env_file:
        raise RuntimeError(f"Env var {github_env_key} not set")

    if not os.path.exists(env_file):
        raise RuntimeError(f"File not found: {env_file}")

    print(f"Setting GitHub env var: {key}={value}")
    with open(env_file, "a") as env_file:
        env_file.write(f'{key}="{value}:${key}"\n')


def add_to_path(value):
    """
    Appends the value to the GitHub path.
    """
    path_file = os.getenv(github_path_key)
    if not path_file:
        raise RuntimeError(f"Env var {github_path_key} not set")

    if not os.path.exists(path_file):
        raise RuntimeError(f"File not found: {path_file}")

    print(f"Adding to GitHub path: {value}")
    with open(path_file, "a") as path_file:
        path_file.write(f"{value}\n")
