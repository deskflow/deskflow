#!/usr/bin/env python3

import os
from lib import env


def main():
    major, minor, patch, stage = env.get_version_info()
    version = f"{major}.{minor}.{patch}-{stage}"
    print(version)


if __name__ == "__main__":
    main()
