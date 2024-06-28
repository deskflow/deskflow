import os, shutil, glob
import lib.cmd_utils as cmd_utils
import lib.env as env

dist_dir = "dist"


def package(filename_base):

    distro, distro_like, _distro_version = env.get_linux_distro()
    if not distro_like:
        distro_like = distro

    extension = None
    generator = None
    if "debian" in distro_like:
        extension = "deb"
        generator = "DEB"
    elif "fedora" in distro_like:
        extension = "rpm"
        generator = "RPM"
    else:
        extension = "sh"
        generator = "STGZ"

    print(f"Building package for distro like {distro_like}")

    original_dir = os.getcwd()
    try:
        os.chdir("build")

        cmd_utils.run(["cpack", "-G", generator])

    finally:
        os.chdir(original_dir)

    os.makedirs(dist_dir, exist_ok=True)

    files = glob.glob(f"build/*.{extension}")
    if not files:
        raise ValueError(f"No .{extension} file found in build directory")

    target = f"{dist_dir}/{filename_base}.{extension}"
    print(f"Copying built .{extension} file to: {target}")
    shutil.copy(files[0], target)
