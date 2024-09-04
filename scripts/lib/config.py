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

import yaml
import lib.env as env
import lib.cmd_utils as cmd_utils

config_file = "config.yaml"
root_key = "config"
deps_key = "dependencies"
command_key = "command"
command_pre_key = "command-pre"
subprojects_key = "subprojects"
arrow = " ➤ "


class ConfigKeyError(RuntimeError):
    def __init__(self, config_file, key):
        self.config_file = config_file
        self.key = key

    def __str__(self):
        return f"Not found in {self.config_file}: {self.key}"


def _get(dict, key, key_parent=None, required=True):
    value = dict.get(key)

    if required and not value:
        key_path = f"{root_key}{arrow}{key_parent}{arrow}{key}" if key_parent else key
        raise ConfigKeyError(config_file, key_path)

    return value


class Config:
    """Reads the project configuration YAML file."""

    def __init__(self):

        with open(config_file, "r") as f:
            data = yaml.safe_load(f)

        self.os_name = env.get_os()

        print("Config for OS:", self.os_name)
        self.root = _get(data, root_key)
        self.os = _get(self.root, self.os_name)

    def get_os_value(self, key, required=True, linux_distro=None):
        if linux_distro:
            # recurse with the linux distro as the key parameter to get the base distro key.
            distro = self.get_os_value(key=linux_distro)
            return _get(distro, key, f"{self.os_name}{arrow}{linux_distro}", required)
        else:
            return _get(self.os, key, self.os_name, required)

    def get_qt_config(self):
        qt_key = "qt"
        qt = self.get_os_deps_value(qt_key)

        parent_key = f"{self.os_name}{arrow}{deps_key}"
        mirror_url = _get(qt, "mirror", parent_key)
        version = _get(qt, "version", parent_key)
        base_dir = _get(qt, "base-dir", parent_key)
        modules = _get(qt, "modules", parent_key, required=False)

        return mirror_url, version, base_dir, modules

    def get_os_deps_value(self, key, required=True, linux_distro=None):
        deps = self.get_os_value(deps_key, required, linux_distro)
        if linux_distro:
            key_parent = f"{self.os_name}{arrow}{linux_distro}{arrow}{deps_key}"
        else:
            key_parent = f"{self.os_name}{arrow}{deps_key}"
        return _get(deps, key, key_parent, required)

    def get_os_deps_command(self, key=command_key, required=True, linux_distro=None):
        command = self.get_os_deps_value(key, required, linux_distro)
        if command:
            return cmd_utils.strip_continuation_sequences(command)
        else:
            return None

    def get_os_subprojects(self):
        distro, _distro_like, _distro_version = env.get_linux_distro()
        return self.get_os_value(subprojects_key, linux_distro=distro, required=False)

    def get_subproject_deps_command(self, subproject_name):
        subprojects = _get(self.root, subprojects_key)
        subproject = _get(subprojects, subproject_name, subprojects_key)
        deps_parent = f"{subprojects_key}{arrow}{subproject_name}"
        deps = _get(subproject, deps_key, deps_parent)

        if env.is_linux():
            distro, _distro_like, _distro_version = env.get_linux_distro()
            if not distro:
                raise RuntimeError("Unable to detect Linux distro")

            command = _get(deps, distro, f"{deps_parent}{arrow}{deps_key}")
        else:
            command = _get(deps, self.os_name, f"{deps_parent}{arrow}{deps_key}")

        return cmd_utils.strip_continuation_sequences(command)

    def get_os_deps_command_pre(self, required=True, linux_distro=None):
        return self.get_os_deps_command(command_pre_key, required, linux_distro)

    def get_windows_ci_config(self):
        choco_ci_key = "ci"
        choco_ci = self.get_os_deps_value(choco_ci_key)

        choco_ci_path = f"{self.os_name}{arrow}{deps_key}{arrow}{choco_ci_key}"
        edit_config = _get(choco_ci, "edit-config", choco_ci_path)
        skip_packages = _get(choco_ci, "skip-packages", choco_ci_path)

        return edit_config, skip_packages
