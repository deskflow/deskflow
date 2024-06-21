#!/usr/bin/env python3

import os
from lib import env

# important: load venv before loading modules that install deps.
env.ensure_in_venv(__file__)

github_key = "GITHUB_ENV"
version_key = "SYNERGY_VERSION"


def main():
    major, minor, patch, stage, _build = env.get_version_info()
    version_value = f"{major}.{minor}.{patch}-{stage}"
    print(f"Version number: {version_value}")

    env_file = os.getenv(github_key)
    if not env_file:
        raise RuntimeError(f"Env var {github_key} not set")

    if not os.path.exists(env_file):
        raise RuntimeError(f"File not found: {env_file}")

    print(f"Setting env var: {version_key}={version_value}")
    with open(env_file, "a") as env_file:
        env_file.write(f"{version_key}={version_value}\n")


if __name__ == "__main__":
    main()
