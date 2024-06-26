from lib import env, cmd_utils

config_file = "config.yml"
root_key = "config"
deps_key = "dependencies"


class ConfigKeyError(RuntimeError):
    def __init__(self, config_file, key):
        self.config_file = config_file
        self.key = key

    def __str__(self):
        return f"Not found in {self.config_file}: {self.key}"


def _get(dict, key, key_parent=None):
    value = dict.get(key)

    if not value:
        key_path = f"{root_key}:{key_parent}:{key}" if key_parent else key
        raise ConfigKeyError(f"Missing key in {config_file}: {key_path}")

    return value


class Config:
    """Reads the project configuration YAML file."""

    def __init__(self):
        env.ensure_module("yaml", "pyyaml")
        import yaml

        with open(config_file, "r") as f:
            data = yaml.safe_load(f)

        self.os_name = env.get_os()
        root = _get(data, root_key)
        self.os = _get(root, self.os_name)

    def get_os_value(self, key):
        return _get(self.os, key, f"{self.os_name}:{key}")

    def get_qt_config(self):
        qt_key = "qt"
        qt = self.get_os_deps_value(qt_key)

        parent_key = f"{self.os_name}:{deps_key}"
        mirror_url = _get(qt, "mirror", parent_key)
        version = _get(qt, "version", parent_key)
        base_dir = _get(qt, "install-dir", parent_key)

        return mirror_url, version, base_dir

    def get_os_deps_value(self, key):
        deps = self.get_os_value(deps_key)
        return _get(deps, key, f"{self.os_name}:{deps_key}")

    def get_deps_command(self):
        deps = self.get_os_value(deps_key)
        command = _get(deps, "command", f"{self.os_name}:{deps_key}")
        return cmd_utils.strip_continuation_sequences(command)

    def get_linux_deps_command(self, distro):
        distro_data = self.get_os_value(distro)
        deps = _get(distro_data, deps_key, f"{self.os_name}:{distro}")
        command = _get(deps, "command", f"{self.os_name}:{distro}:{deps_key}")
        return cmd_utils.strip_continuation_sequences(command)

    def get_choco_ci_config(self):
        choco_ci_key = "choco-ci"
        choco_ci = self.get_os_deps_value(choco_ci_key)

        choco_ci_path = f"{self.os_name}:{deps_key}:{choco_ci_key}"
        edit_config = _get(choco_ci, "edit-config", choco_ci_path)
        skip_packages = _get(choco_ci, "skip-packages", choco_ci_path)

        return edit_config, skip_packages
