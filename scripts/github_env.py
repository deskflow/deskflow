#!/usr/bin/env python3

import argparse
import lib.env as env
import lib.github as github

qt_version_key = "QT_VERSION"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--set-qt-version",
        action="store_true",
        help=f"Set {qt_version_key} env var",
    )
    args = parser.parse_args()

    # important: load venv before loading modules that install deps.
    env.ensure_in_venv(__file__)

    from lib.config import Config

    if args.set_qt_version:
        config = Config()
        _, qt_version, _, _ = config.get_qt_config()
        github.set_env(qt_version_key, qt_version)
    else:
        raise RuntimeError("No option selected")


if __name__ == "__main__":
    main()
