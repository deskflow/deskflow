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
    result = cmd_utils.run(
        [sudo, meson_bin, "install", "-C", build_dir],
        print_cmd=True,
        get_output=True,
        check=False,
    )
    if result.stdout:
        print("Install output: " + result.stdout)

    if result.stderr:
        print("Install error: " + result.stderr)

    if result.returncode != 0:
        raise RuntimeError("Install failed")
