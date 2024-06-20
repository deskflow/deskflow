#!/usr/bin/env python3

from lib import env

env.ensure_in_venv("build/mac_package", __file__)
env.ensure_module("dotenv", "python-dotenv")

from dotenv import load_dotenv
import os

version_env = "build/.env.version"


def main():
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
