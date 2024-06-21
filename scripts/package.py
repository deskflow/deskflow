#!/usr/bin/env python3

import os
from lib import env

env.ensure_in_venv(__file__)
env.ensure_module("dotenv", "python-dotenv")
from dotenv import load_dotenv  # type: ignore

env_file = ".env"
version_env = "build/.env.version"


def main():
    load_dotenv(dotenv_path=env_file)

    if not os.path.isfile(version_env):
        raise RuntimeError(f"Version file not found: {version_env}")

    load_dotenv(dotenv_path=version_env)

    major = os.getenv("SYNERGY_VERSION_MAJOR")
    minor = os.getenv("SYNERGY_VERSION_MINOR")
    patch = os.getenv("SYNERGY_VERSION_PATCH")
    build = os.getenv("SYNERGY_VERSION_BUILD")
    stage = os.getenv("SYNERGY_VERSION_STAGE")

    version = f"{major}.{minor}.{patch}-{stage}+build-{build}"
    print(f"Synergy version: {version}")

    if env.is_windows():
        windows_package()
    elif env.is_mac():
        mac_package()
    elif env.is_linux():
        linux_package()
    else:
        raise RuntimeError(f"Unsupported platform: {env.get_os()}")


def windows_package():
    print("TODO: Windows packaging")


def mac_package():
    from lib import mac

    mac.package()


def linux_package():
    print("TODO: Linux packaging")


if __name__ == "__main__":
    main()
