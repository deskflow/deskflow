import os, shutil, glob
import lib.cmd_utils as cmd_utils
import lib.env as env

dist_dir = "dist"


def package(filename_base):
    build_extra_packages = env.get_env_bool("LINUX_EXTRA_PACKAGES", False)

    distro, distro_like, _distro_version = env.get_linux_distro()
    if not distro_like:
        distro_like = distro

    build_package(filename_base, distro_like)

    if build_extra_packages:
        build_package(filename_base, build_tgz=True)
        build_package(filename_base, build_stgz=True)


def build_package(filename_base, distro_like=None, build_tgz=False, build_stgz=False):

    extension = None
    generator = None

    if build_tgz:
        generator = "TGZ"
        extension = "tar.gz"
        print("Building package for Linux (tar.gz archive)")

    elif build_stgz:
        generator = "STGZ"
        extension = "sh"
        print("Building package for Linux (self-extracting tar.gz)")
    else:

        print(f"Building package for distro like {distro_like}")

        if "debian" in distro_like:
            generator = "DEB"
            extension = "deb"
        elif "fedora" in distro_like:
            generator = "RPM"
            extension = "rpm"

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
