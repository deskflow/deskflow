#!/usr/bin/env python3

import os, sys
from lib import env

env.ensure_in_venv("build/python", __file__)
env.ensure_module("dotenv", "python-dotenv")
from dotenv import load_dotenv

version_env = "build/.env.version"


def main():
    print(f"Operating system: {env.get_os()}")

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


if __name__ == "__main__":
    main()
