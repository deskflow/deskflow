#!/usr/bin/env python3

import os
from lib import env

# important: load venv before loading modules that install deps.
env.ensure_in_venv(__file__)

env_file = ".env"
version_env = "build/.env.version"
package_filename_product = "synergy"


def main():
    env.ensure_module("dotenv", "python-dotenv")
    from dotenv import load_dotenv  # type: ignore

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
    filename_base = get_filename_base(version)
    print(f"Package filename base: {filename_base}")

    if env.is_windows():
        windows_package(filename_base)
    elif env.is_mac():
        mac_package(filename_base)
    elif env.is_linux():
        linux_package(filename_base)
    else:
        raise RuntimeError(f"Unsupported platform: {env.get_os()}")


def get_filename_base(version):
    os = env.get_os()
    arch = env.get_arch()
    return f"{package_filename_product}-{version}-{os}-{arch}"


def windows_package(filename_base):
    """TODO: Windows packaging"""
    pass


def mac_package(filename_base):
    from lib import mac

    mac.package(filename_base)


def linux_package(filename_base):
    """TODO: Linux packaging"""
    pass


if __name__ == "__main__":
    main()
