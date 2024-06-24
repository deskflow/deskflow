#!/usr/bin/env python3

import os, argparse
from lib import env
from lib.config import Config

# important: load venv before loading modules that install deps.
env.ensure_in_venv(__file__)

github_key = "GITHUB_ENV"
app_version_key = "SYNERGY_VERSION"
qt_version_key = "QT_VERSION"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--set-app-version",
        action="store_true",
        help=f"Set {app_version_key} env var",
    )
    parser.add_argument(
        "--set-qt-version",
        action="store_true",
        help=f"Set {qt_version_key} env var",
    )
    args = parser.parse_args()

    if args.set_app_version:
        major, minor, patch, stage, _build = env.get_version_info()
        version_value = f"{major}.{minor}.{patch}-{stage}"
        set_github_env(app_version_key, version_value)
    elif args.set_qt_version:
        config = Config()
        _qt_mirror, qt_version, _qt_install_dir = config.get_qt_config()
        set_github_env(qt_version_key, qt_version)
    else:
        raise RuntimeError("No option selected")


def set_github_env(key, value):
    """
    Appends the key=value pair to the GitHub environment file.
    """
    env_file = os.getenv(github_key)
    if not env_file:
        raise RuntimeError(f"Env var {github_key} not set")

    if not os.path.exists(env_file):
        raise RuntimeError(f"File not found: {env_file}")

    print(f"Setting env var: {key}={value}")
    with open(env_file, "a") as env_file:
        env_file.write(f"{key}={value}\n")


if __name__ == "__main__":
    main()
