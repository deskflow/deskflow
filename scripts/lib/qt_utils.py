import os
import lib.cmd_utils as cmd_utils
import lib.env as env
import glob


class Qt:
    def __init__(
        self, mirror_url, version, base_dir, modules, os_name, compiler, tool_dir
    ):
        self.mirror_url = mirror_url
        self.version = version
        self.base_dir = base_dir
        self.modules = modules
        self.os_name = os_name
        self.compiler = compiler
        self.tool_dir = tool_dir
        self.dir_pattern = f"{self.base_dir}{os.sep}{self.version}*/{self.tool_dir}"

    def get_install_dir(self):
        match = glob.glob(self.dir_pattern)
        return os.path.abspath(match[0]) if match else None

    def install(self):
        """Install Qt."""

        if self.get_install_dir():
            print(f"Skipping Qt, already installed at: {self.dir_pattern}")
            return

        args = [env.get_python_executable(), "-m", "aqt", "install-qt"]
        args.extend(["--outputdir", self.base_dir])
        args.extend(["--base", self.mirror_url])
        args.extend([self.os_name, "desktop", str(self.version), self.compiler])

        if self.modules:
            args.extend(["-m"] + self.modules)

        print(args)
        cmd_utils.run(
            args,
            print_cmd=True,
        )

        if not self.get_install_dir():
            raise RuntimeError(
                f"Qt was not installed, path not found: {self.dir_pattern}"
            )


class WindowsQt(Qt):
    def __init__(self, mirror_url, version, base_dir, modules):
        super().__init__(
            mirror_url,
            version,
            base_dir,
            modules,
            "windows",
            "win64_msvc2019_64",
            "msvc2019_64",
        )


class MacQt(Qt):
    def __init__(self, mirror_url, version, base_dir, modules):
        super().__init__(
            mirror_url,
            version,
            base_dir,
            modules,
            "mac",
            "clang_64",
            "macos",
        )
