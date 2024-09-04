# Synergy -- mouse and keyboard sharing utility
# Copyright (C) 2024 Symless Ltd.
#
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file LICENSE that should have accompanied this file.
#
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import os

github_env_key = "GITHUB_ENV"
github_path_key = "GITHUB_PATH"


def set_env_var(key, value):
    """
    Appends the key=value pair to the GitHub environment file.
    """
    env_file = os.getenv(github_env_key)
    if not env_file:
        raise RuntimeError(f"Env var {github_env_key} not set")

    if not os.path.exists(env_file):
        raise RuntimeError(f"File not found: {env_file}")

    print(f"Setting GitHub env var: {key}={value}")
    with open(env_file, "a") as env_file:
        env_file.write(f"{key}={value}\n")


def add_to_path(value):
    """
    Appends the value to the GitHub path.
    """
    path_file = os.getenv(github_path_key)
    if not path_file:
        raise RuntimeError(f"Env var {github_path_key} not set")

    if not os.path.exists(path_file):
        raise RuntimeError(f"File not found: {path_file}")

    print(f"Adding to GitHub path: {value}")
    with open(path_file, "a") as path_file:
        path_file.write(f"{value}\n")
