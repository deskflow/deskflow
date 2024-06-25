#!/usr/bin/env python3

import argparse
from lib import env, github
from lib.config import Config

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

    # important: load venv before loading modules that install deps.
    env.ensure_in_venv(__file__)

    if args.set_app_version:
        version = env.get_app_version()
        github.set_env(app_version_key, version)
    elif args.set_qt_version:
        config = Config()
        _qt_mirror, qt_version, _qt_install_dir = config.get_qt_config()
        github.set_env(qt_version_key, qt_version)
    else:
        raise RuntimeError("No option selected")


if __name__ == "__main__":
    main()
