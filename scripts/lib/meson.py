import lib.cmd_utils as cmd_utils
import lib.env as env
import os

build_dir = "build/meson"
meson_bin = env.get_python_executable("meson")


def setup():
    reconfigure = "--reconfigure" if os.path.exists(build_dir) else ""
    cmd_utils.run([meson_bin, "setup", build_dir, reconfigure], print_cmd=True)


def compile():
    cmd_utils.run([meson_bin, "compile", "-C", build_dir], print_cmd=True)


def install():
    has_sudo = cmd_utils.has_command("sudo")
    sudo = "sudo" if has_sudo else ""
    cmd_utils.run(
        [sudo, meson_bin, "install", "-C", build_dir, "--verbose"], print_cmd=True
    )
