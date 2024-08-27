import os, shutil, glob
import lib.cmd_utils as cmd_utils
import lib.env as env
from enum import Enum, auto


class PackageType(Enum):
    DISTRO = auto()
    TGZ = auto()


dist_dir = "dist"
build_dir = "build"
package_name = "synergy"
test_cmd = "synergys --version"


def run_command(command, check=True, add_env=None):

    has_sudo = cmd_utils.has_command("sudo")
    if "sudo" in command and not has_sudo:
        # assume we're running as root if sudo is not found (common on older distros).
        # a space char is intentionally added after "sudo" for intentionality.
        # possible limitation with stripping "sudo" is that if any packages with "sudo" in the
        # name are added to the list (probably very unlikely), this will have undefined behavior.
        print("The 'sudo' command was not found, stripping sudo from command")
        command = command.replace("sudo ", "").strip()

    cmd_utils.run(command, check, shell=True, print_cmd=True, add_env=add_env)


def package(filename_base, package_type: PackageType):

    extension, cmd = get_package_info(package_type)
    run_package_cmd(cmd)
    package_filename = get_package_filename(extension)
    target_file = f"{filename_base}.{extension}"
    target_path = copy_to_dist_dir(package_filename, target_file)

    if package_type == PackageType.DISTRO:
        test_install(target_path)


def get_package_info(package_type: PackageType):

    command = None
    cpack_generator = None
    file_extension = None

    if package_type == PackageType.TGZ:
        cpack_generator = "TGZ"
        file_extension = "tar.gz"

    elif package_type == PackageType.DISTRO:

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
            raise RuntimeError(f"Linux distro not yet supported: {distro}")

    if not cpack_generator and not command:
        raise RuntimeError("No package generator or command found")

    if cpack_generator:
        command = ["cpack", "-G", cpack_generator]

    return file_extension, command


def run_package_cmd(command):
    package_user = env.get_env("LINUX_PACKAGE_USER", required=False)
    if package_user:
        cmd_utils.run(
            ["sudo", "chown", "-R", package_user, "build"], check=True, print_cmd=True
        )
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

    return target_path


def test_install(package_file):

    distro, distro_like, _distro_version = env.get_linux_distro()
    if not distro_like:
        distro_like = distro

    install_pre = None
    remove_pre = None
    if "debian" in distro_like:
        install_pre = ["apt", "install", "-f", "-y"]
        remove_pre = ["apt", "remove", "-y"]
    elif "fedora" in distro_like:
        install_pre = ["dnf", "install", "-y"]
        remove_pre = ["dnf", "remove", "-y"]
    elif "opensuse" in distro_like:
        install_pre = ["zypper", "--no-gpg-checks", "install", "-y"]
        remove_pre = ["zypper", "remove", "-y"]
    elif "arch" in distro_like:
        install_pre = ["pacman", "-U", "--noconfirm"]
        remove_pre = ["pacman", "-R", "--noconfirm"]
    else:
        raise RuntimeError(f"Linux distro not yet supported: {distro}")

    has_sudo = cmd_utils.has_command("sudo")
    sudo = ["sudo"] if has_sudo else []

    print("Testing installation...")
    cmd_utils.run(
        sudo + install_pre + [f"./{package_file}"],
        check=True,
        print_cmd=True,
    )

    try:
        cmd_utils.run(test_cmd, shell=True, check=True, print_cmd=True)
    except Exception:
        raise RuntimeError("Unable to verify version")
    finally:
        cmd_utils.run(sudo + remove_pre + [package_name], check=True, print_cmd=True)
        print("Installation test passed")
