import os, shutil, glob
import lib.cmd_utils as cmd_utils
import lib.env as env
import lib.checksums as checksums

dist_dir = "dist"
build_dir = "build"


def package(filename_base, build_distro=True, build_tgz=False, build_stgz=False):

    extension, generator, post_cmd = get_package_info(
        build_distro, build_tgz, build_stgz
    )

    run_cpack(generator)
    package_filename = get_package_filename(extension)

    checksums.generate_sha256_file(package_filename)

    if post_cmd:
        cwd = os.getcwd()
        try:
            os.chdir(build_dir)
            cmd_utils.run(post_cmd, check=True, print_cmd=True, shell=True)
        finally:
            os.chdir(cwd)

    target_file = f"{filename_base}.{extension}"
    copy_to_dist_dir(package_filename, target_file)
    copy_to_dist_dir(f"{package_filename}.sha256", f"{target_file}.sha256")


def get_package_info(build_distro, build_tgz, build_stgz):

    extension = None
    generator = None
    post_cmd = None

    if build_tgz:
        generator = "TGZ"
        extension = "tar.gz"
        print("Building package for Linux (tar.gz archive)")

    elif build_stgz:
        generator = "STGZ"
        extension = "sh"
        print("Building package for Linux (self-extracting tar.gz)")

    elif build_distro:

        distro, distro_like, _distro_version = env.get_linux_distro()
        if not distro_like:
            distro_like = distro

        print(f"Building package for distro like {distro_like}")

        if "debian" in distro_like:
            generator = "DEB"
            extension = "deb"
        elif "fedora" in distro_like or "opensuse" in distro_like:
            generator = "RPM"
            extension = "rpm"
        elif "arch" in distro_like:
            generator = "TGZ"
            extension = "tar.gz"
            post_cmd = "makepkg -si"
        else:
            raise RuntimeError(f"Linux distro not yet supported: {distro_like}")

    return extension, generator, post_cmd


def run_cpack(generator):

    original_dir = os.getcwd()
    try:
        os.chdir("build")

        cmd_utils.run(
            ["cpack", "-G", generator], check=True, print_cmd=True, shell=True
        )

    finally:
        os.chdir(original_dir)


def get_package_filename(extension):
    files = glob.glob(f"build/*.{extension}")

    if not files:
        raise ValueError(f"No .{extension} file found in build directory")

    return files[0]


def copy_to_dist_dir(source_file, target_file):
    os.makedirs(dist_dir, exist_ok=True)

    target_path = f"{dist_dir}/{target_file}"
    print(f"Copying to: {target_path}")
    shutil.copy(source_file, target_path)
