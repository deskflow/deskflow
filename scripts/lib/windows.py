import ctypes, sys, os, shutil
import xml.etree.ElementTree as ET
import lib.cmd_utils as cmd_utils
import lib.env as env
from lib.certificate import Certificate

msbuild_cmd = "msbuild"
signtool_cmd = "signtool"
certutil_cmd = "certutil"
runner_temp_env_var = "RUNNER_TEMP"
dist_dir = "dist"
build_dir = "build"
wix_solution_file = f"{build_dir}/installer/Synergy.sln"
installer_file = f"{build_dir}/installer/bin/Release/Synergy.msi"


def relaunch_as_admin(script):
    args = " ".join(sys.argv[1:])
    command = f"{script} --pause-on-exit {args}"
    print(f"Re-launching script as admin: {command}")
    ctypes.windll.shell32.ShellExecuteW(None, "runas", sys.executable, command, None, 1)


def is_admin():
    """Returns True if the current process has admin privileges."""
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except ctypes.WinError:
        return False


def set_env_var(name, value):
    """
    Sets or updates an environment variable. Appends the value if it doesn't already exist.

    Args:
    name (str): The name of the environment variable.
    value (str): The value of the environment variable.
    """

    current_value = os.getenv(name, "")

    if value not in current_value:
        new_value = f"{current_value}{os.pathsep}{value}" if current_value else value
        os.environ[name] = new_value
        print(f"Setting environment variable: {name}={value}")
        cmd_utils.run(["setx", name, new_value], check=True, shell=True, print_cmd=True)


def package(filename_base):
    cert_env_key = "WINDOWS_PFX_CERTIFICATE"
    cert_base64 = env.get_env(cert_env_key, required=False)
    if cert_base64:
        cert_password = env.get_env("WINDOWS_PFX_PASSWORD")
        sign_binaries(cert_base64, cert_password)

    build_msi(filename_base)

    if cert_base64:
        sign_msi(filename_base, cert_base64, cert_password)
    else:
        print(f"Skipped code signing, env var not set: {cert_env_key}")


def assert_vs_cmd(cmd):
    has_cmd = cmd_utils.has_command(cmd)
    if not has_cmd:
        raise RuntimeError(
            f"The '{cmd}' command was not found, "
            "re-run from 'Developer Command Prompt for VS'"
        )


def build_msi(filename_base):
    print("Building MSI installer...")
    configuration = "Release"
    platform = "x64"

    assert_vs_cmd(msbuild_cmd)
    cmd_utils.run(
        [
            msbuild_cmd,
            wix_solution_file,
            f"/p:Configuration={configuration}",
            f"/p:Platform={platform}",
        ],
        shell=True,
        print_cmd=True,
    )

    path = get_package_path(filename_base)
    print(f"Copying MSI installer to {dist_dir}")
    os.makedirs(dist_dir, exist_ok=True)
    shutil.copy(installer_file, path)


def get_package_path(filename_base):
    return f"{dist_dir}/{filename_base}.msi"


def sign_binaries(cert_base64, cert_password):
    exe_pattern = f"{build_dir}/bin/*.exe"
    run_codesign(exe_pattern, cert_base64, cert_password)


def sign_msi(filename_base, cert_base64, cert_password):
    path = get_package_path(filename_base)
    run_codesign(path, cert_base64, cert_password)


def run_codesign(path, cert_base64, cert_password):
    time_server = "http://timestamp.digicert.com"
    hashing_algorithm = "SHA256"

    with Certificate(cert_base64, "pfx") as cert_path:
        print("Signing MSI installer...")
        assert_vs_cmd(signtool_cmd)

        # WARNING: contains private key password, never print this command
        cmd_utils.run(
            [
                signtool_cmd,
                "sign",
                "/f",
                cert_path,
                "/p",
                cert_password,
                "/t",
                time_server,
                "/fd",
                hashing_algorithm,
                path,
            ]
        )


class WindowsChoco:
    """Chocolatey for Windows."""

    def install(self, command, ci_env):
        """Installs packages using Chocolatey."""
        if ci_env:
            # don't show noisy choco progress bars in ci env
            cmd_utils.run(
                f"{command} --no-progress",
                shell=True,
                print_cmd=True,
            )
        else:
            self.ensure_choco_installed()

            cmd_utils.run(
                command,
                shell=True,
                print_cmd=True,
            )

    def config_ci_cache(self):
        """Configures Chocolatey cache for CI."""

        runner_temp = os.environ.get(runner_temp_env_var)
        if runner_temp:
            # sets the choco cache dir, which should match the dir in the ci cache action.
            key_arg = '--name="cacheLocation"'
            value_arg = f'--value="{runner_temp}/choco"'
            cmd_utils.run(
                ["choco", "config", "set", key_arg, value_arg],
                print_cmd=True,
            )
        else:
            print(f"Warning: CI environment variable {runner_temp_env_var} not set")

    def remove_from_config(self, choco_config_file, remove_packages):
        """Removes a package from the Chocolatey configuration."""

        tree = ET.parse(choco_config_file)
        root = tree.getroot()
        for remove in remove_packages:
            for package in root.findall("package"):
                if package.get("id") == remove:
                    root.remove(package)
                    print(f"Removed package from choco config: {remove}")

        tree.write(choco_config_file)

    def ensure_choco_installed(self):
        if cmd_utils.has_command("choco"):
            return

        if not cmd_utils.has_command("winget"):
            print("The winget command was not found", file=sys.stderr)
            sys.exit(1)

        print("The choco command was not found, installing Chocolatey...")
        cmd_utils.run(
            "winget install chocolatey",
            check=False,
            shell=True,
            print_cmd=True,
        )

        if not cmd_utils.has_command("choco"):
            print(
                "The choco command was still not found, please re-run this script...",
                file=sys.stderr,
            )
            sys.exit(1)
