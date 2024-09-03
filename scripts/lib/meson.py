import lib.cmd_utils as cmd_utils
import lib.env as env
import os

build_dir = "build/meson"
meson_bin = env.get_python_executable("meson")


def setup(no_system_list):
    cmd = [meson_bin, "setup", build_dir]

    if env.is_windows():
        cmd.append("-Dsystem_gtest=false")

    for subproject in no_system_list:
        cmd.append(f"-Dsystem_{subproject}=false")

    if os.path.exists(build_dir):
        cmd.append("--reconfigure")

    cmd_utils.run(cmd, print_cmd=True)


def compile():
    cmd_utils.run([meson_bin, "compile", "-C", build_dir], print_cmd=True)


def install():
    cmd = [meson_bin, "install", "-C", build_dir]

    has_sudo = cmd_utils.has_command("sudo")
    if has_sudo:
        cmd.insert(0, "sudo")

    cmd_utils.run(cmd, print_cmd=True)
