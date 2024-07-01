import os, shutil, glob
import lib.cmd_utils as cmd_utils
import lib.env as env

dist_dir = "dist"
build_dir = "build"


def package(filename_base, build_distro=True, build_tgz=False, build_stgz=False):

    extension, cmd = get_package_info(build_distro, build_tgz, build_stgz)
    run_package_cmd(cmd)
    package_filename = get_package_filename(extension)
    target_file = f"{filename_base}.{extension}"
    copy_to_dist_dir(package_filename, target_file)


def get_package_info(build_distro, build_tgz, build_stgz):

    command = None
    cpack_generator = None
    file_extension = None

    if build_tgz:
        cpack_generator = "TGZ"
        file_extension = "tar.gz"

    elif build_stgz:
        cpack_generator = "STGZ"
        file_extension = "sh"

    elif build_distro:

        distro, distro_like, _distro_version = env.get_linux_distro()
        if not distro_like:
            distro_like = distro

        if "debian" in distro_like:
            cpack_generator = "DEB"
            file_extension = "deb"
        elif "fedora" in distro_like or "opensuse" in distro_like:
            cpack_generator = "RPM"
            file_extension = "rpm"
        elif "arch" in distro_like:
            command = ["makepkg", "-s"]
            file_extension = "pkg.tar.zst"
        else:
            raise RuntimeError(f"Linux distro not yet supported: {distro_like}")

    if not cpack_generator and not command:
        raise RuntimeError("No package generator or command found")

    if cpack_generator:
        command = ["cpack", "-G", cpack_generator]

    return file_extension, command


def run_package_cmd(command):
    package_user = env.get_env("LINUX_PACKAGE_USER", required=False)
    if package_user:
        command = ["sudo", "-u", package_user] + command

    cwd = os.getcwd()
    try:
        os.chdir("build")
        cmd_utils.run(command, check=True, print_cmd=True)
    finally:
        os.chdir(cwd)


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
