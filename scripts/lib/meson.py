import lib.cmd_utils as cmd_utils
import lib.env as env
import os

build_dir = "build/meson"
meson_bin = env.get_python_executable("meson")


def setup(no_system_list):
    cmd = [meson_bin, "setup", build_dir]

    if env.is_windows():
        cmd.append("-Dsystem_gtest=false")

    for subproject in no_system_list or []:
        cmd.append(f"-Dsystem_{subproject}=false")

    # This might be a bit rude, but Meson seems to cache a lot (like CMake),
    # so wiping every time is the easiest way to ensure that the build is clean.
    # Plus, the way we're using Meson (at the moment) is just for satisfying
    # dependencies, so this script is run infrequently enough to not matter.
    if os.path.exists(build_dir):
        cmd.append("--wipe")

    cmd_utils.run(cmd, print_cmd=True)


def compile():
    cmd_utils.run([meson_bin, "compile", "-C", build_dir], print_cmd=True)


def install():
    cmd = [meson_bin, "install", "-C", build_dir]

    has_sudo = cmd_utils.has_command("sudo")
    if has_sudo:
        cmd.insert(0, "sudo")

    cmd_utils.run(cmd, print_cmd=True)
