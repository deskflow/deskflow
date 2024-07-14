import os

github_key = "GITHUB_ENV"


def set_env_var(key, value):
    """
    Appends the key=value pair to the GitHub environment file.
    """
    env_file = os.getenv(github_key)
    if not env_file:
        raise RuntimeError(f"Env var {github_key} not set")

    if not os.path.exists(env_file):
        raise RuntimeError(f"File not found: {env_file}")

    print(f"Setting GitHub env var: {key}={value}")
    with open(env_file, "a") as env_file:
        env_file.write(f'{key}="{value}:${key}"\n')
