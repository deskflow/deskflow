from lib import env, cmd_utils

env.ensure_module("yaml", "pyyaml")
import yaml

config_file = "config.yml"
deps_key = "dependencies"


class ConfigError(RuntimeError):
    pass


class Config:
    """Reads the project configuration YAML file."""

    def __init__(self):
        with open(config_file, "r") as f:
            data = yaml.safe_load(f)

        self.os_name = env.get_os()
        root_key = "config"
        try:
            root = data[root_key]
        except KeyError:
            raise ConfigError(f"Nothing found in {config_file} for: {root_key}")

        try:
            self.os = root[self.os_name]
        except KeyError:
            raise ConfigError(f"Nothing found in {config_file} for: {self.os_name}")

    def get_os_value(self, key):
        try:
            return self.os[key]
        except KeyError:
            raise ConfigError(
                f"Nothing found in {config_file} for: {self.os_name}:{key}"
            )

    def get_qt_config(self):
        qt = self.get_os_deps_value("qt")

        try:
            mirror_url = qt["mirror"]
        except KeyError:
            raise ConfigError(f"Qt mirror not set in {self.config_file}")

        try:
            default_version = qt["version"]
        except KeyError:
            raise ConfigError(f"Qt version not set in {self.config_file}")

        try:
            default_base_dir = qt["install-dir"]
        except KeyError:
            raise ConfigError(f"Qt install-dir not set in {self.config_file}")

        return mirror_url, default_version, default_base_dir

    def get_os_deps_value(self, key):
        deps_key = "dependencies"
        deps = self.get_os_value(deps_key)
        try:
            return deps[key]
        except KeyError:
            raise ConfigError(
                f"Nothing found in {config_file} for: {self.os_name}:{deps_key}:{key}"
            )

    def get_deps_command(self):
        dependencies = self.get_os_value("dependencies")
        try:
            command = dependencies["command"]
        except KeyError:
            raise ConfigError(
                f"No dependencies command found in {config_file} for: {self.os_name}"
            )
        return cmd_utils.strip_continuation_sequences(command)

    def get_linux_deps_command(self, distro):
        distro_data = self.get_os_value(distro)

        try:
            deps = distro_data[deps_key]
        except KeyError:
            raise ConfigError(
                f"No dependencies config found in {config_file} for: {distro}"
            )

        try:
            command = deps["command"]
            return cmd_utils.strip_continuation_sequences(command)
        except KeyError:

            raise ConfigError(
                f"No dependencies command found in {config_file} for: {self.os_name}:{distro}"
            )

    def get_choco_config(self):
        ci = self.get_os_deps_value("ci")
        try:
            ci_skip = ci["skip"]
            choco_config_file = ci_skip["edit-config"]
            remove_packages = ci_skip["packages"]
        except KeyError:
            raise ConfigError(f"Bad structure in {config_file} under: ci")
        return choco_config_file, remove_packages
