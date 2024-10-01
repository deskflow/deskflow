# Deskflow -- mouse and keyboard sharing utility
# Copyright (C) 2024 Symless Ltd.
#
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file LICENSE that should have accompanied this file.
#
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import os, shutil, glob, sys
import lib.cmd_utils as cmd_utils
import lib.env as env
from enum import Enum, auto

BUILD_ROOT_DIR = "build"


class PackageType(Enum):
    DISTRO = auto()
    TGZ = auto()


def run_command(command, check=True):

    has_sudo = cmd_utils.has_command("sudo")
    if "sudo" in command and not has_sudo:
        # assume we're running as root if sudo is not found (common on older distros).
        # a space char is intentionally added after "sudo" for intentionality.
        # possible limitation with stripping "sudo" is that if any packages with "sudo" in the
        # name are added to the list (probably very unlikely), this will have undefined behavior.
        print("The 'sudo' command was not found, stripping sudo from command")
        command = command.replace("sudo ", "").strip()

    cmd_utils.run(command, check, shell=True, print_cmd=True)


def package(
    filename_base,
    dist_dir,
    test_cmd,
    package_name,
    package_type: PackageType,
    leave_test_installed=False,
):
    working_dir = BUILD_ROOT_DIR
    extension, cmd = get_package_build_info(
        package_type,
    )
    run_package_cmd(cmd, working_dir)

    package_filename = get_package_filename(extension, working_dir)
    target_file = f"{filename_base}.{extension}"
    target_path = copy_to_dist_dir(package_filename, dist_dir, target_file)

    if package_type == PackageType.DISTRO:
        test_install(
            target_path, package_name, test_cmd, remove_test=not leave_test_installed
        )


def get_package_build_info(package_type: PackageType):

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


def run_package_cmd(command, working_dir):
    package_user = env.get_env("LINUX_PACKAGE_USER", required=False)
    if package_user:
        cmd_utils.run(
            ["sudo", "chown", "-R", package_user, working_dir],
            check=True,
            print_cmd=True,
        )
        command = ["sudo", "-u", package_user] + command

    cwd = os.getcwd()
    try:
        os.chdir(working_dir)
        cmd_utils.run(command, check=True, print_cmd=True)
    finally:
        os.chdir(cwd)


def get_package_filename(extension, working_dir):
    files = glob.glob(f"{working_dir}/*.{extension}")

    if not files:
        raise ValueError(
            f"No .{extension} file found in build directory: {working_dir}"
        )

    return files[0]


def copy_to_dist_dir(source_file, dist_dir, target_file):
    os.makedirs(dist_dir, exist_ok=True)

    target_path = f"{dist_dir}/{target_file}"
    print(f"Copying to: {target_path}")
    shutil.copy(source_file, target_path)

    return target_path


def test_install(package_file, package_name, test_cmd, remove_test=True):

    distro, distro_like, _distro_version = env.get_linux_distro()
    if not distro_like:
        distro_like = distro

    install_base = None
    list_cmd = None
    remove_base = None
    if "debian" in distro_like:
        install_base = ["apt", "install", "-f", "-y"]
        remove_base = ["apt", "remove", "-y"]
        list_cmd = ["dpkg", "-L", package_name]
    elif "fedora" in distro_like:
        install_base = ["dnf", "install", "-y"]
        remove_base = ["dnf", "remove", "-y"]
        list_cmd = ["rpm", "-ql", package_name]
    elif "opensuse" in distro_like:
        install_base = ["zypper", "--no-gpg-checks", "install", "-y"]
        remove_base = ["zypper", "remove", "-y"]
        list_cmd = ["rpm", "-ql", package_name]
    elif "arch" in distro_like:
        install_base = ["pacman", "-U", "--noconfirm"]
        remove_base = ["pacman", "-R", "--noconfirm"]
        list_cmd = ["pacman", "-Ql", package_name]
    else:
        raise RuntimeError(f"Linux distro not yet supported: {distro}")

    has_sudo = cmd_utils.has_command("sudo")
    sudo = ["sudo"] if has_sudo else []

    print("Testing installation...")
    cmd_utils.run(
        sudo + install_base + [f"./{package_file}"],
        check=True,
        print_cmd=True,
    )

    print("Listing installed files...")
    cmd_utils.run(sudo + list_cmd, check=True, print_cmd=True)

    try:
        cmd_utils.run(test_cmd, shell=True, check=True, print_cmd=True)
    except Exception:
        raise RuntimeError("Unable to verify version")
    finally:
        if remove_test:
            cmd_utils.run(
                sudo + remove_base + [package_name], check=True, print_cmd=True
            )
        else:
            print("Leaving test package installed")

        print("Installation test passed")


def is_package_available(package):
    distro, distro_like, _distro_version = env.get_linux_distro()
    if not distro_like:
        distro_like = distro

    if "debian" in distro_like:
        command = ["apt-cache", "show", package]
    elif "fedora" in distro_like:
        command = ["dnf", "info", package]
    elif "opensuse" in distro_like:
        command = ["zypper", "info", package]
    elif "arch" in distro_like:
        command = ["pacman", "-Si", package]
    else:
        raise RuntimeError(f"Linux distro not yet supported: {distro}")

    result = cmd_utils.run(command, check=False, print_cmd=True, get_output=True)
    if result.stderr:
        print(result.stderr, file=sys.stderr)
    return result.returncode == 0
