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

import dmgbuild  # type: ignore
import os, time, json, shutil, sys
import lib.cmd_utils as cmd_utils
import lib.env as env
from lib.certificate import Certificate

CERT_P12_ENV = "APPLE_P12_CERTIFICATE"
NOTARY_USER_ENV = "APPLE_NOTARY_USER"
CODESIGN_ENV = "APPLE_CODESIGN_ID"
SHELL_RC = "~/.zshrc"
SETTINGS_FILE = "deploy/dist/mac/dmgbuild/settings.py"
SECURITY_PATH = "/usr/bin/security"
SUDO_PATH = "/usr/bin/sudo"
NOTARYTOOL_PATH = "/usr/bin/notarytool"
CODESIGN_PATH = "/usr/bin/codesign"
XCODE_SELECT_PATH = "/usr/bin/xcode-select"
KEYCHAIN_PATH = "/Library/Keychains/System.keychain"


def set_env_var(name, value):
    """
    Adds to an environment variable in the shell rc file.

    Returns True if the variable was added, False if it already exists.
    """
    text = f'export {name}="{value}:${name}"'
    file = os.path.expanduser(SHELL_RC)
    if os.path.exists(file):
        with open(file, "r") as f:
            if text in f.read():
                return False

    print(f"Setting environment variable: {name}={name}")
    with open(file, "a") as f:
        f.write(f"\n{text}\n")
        print(f"Appended to {SHELL_RC}: {text}")
        return True


def package(filename_base, source_dir, build_dir, dist_dir, product_name):
    """
    Package the application for macOS.
    The app bundle must be signed, or an error will occur:
    > EXC_BAD_ACCESS (SIGKILL (Code Signature Invalid))
    An "Apple Development" certificate is sufficient for local development.
    """

    (
        codesign_id,
        cert_base64,
        cert_password,
        notary_user,
        notary_password,
        notary_team_id,
    ) = package_env_vars()

    if cert_base64:
        install_certificate(cert_base64, cert_password)
    else:
        print(
            f"Warning: Skipped certificate installation, env var {CERT_P12_ENV} not set",
            file=sys.stderr,
        )

    bundle_source_dir = os.path.join(
        build_dir, os.path.join("bundle", product_name + ".app")
    )
    build_bundle(bundle_source_dir)

    if codesign_id:
        sign_bundle(bundle_source_dir, codesign_id)
    else:
        print(
            f"Warning: Skipped code signing, env var {CODESIGN_ENV} not set",
            file=sys.stderr,
        )

    dmg_path = build_dmg(
        bundle_source_dir, filename_base, source_dir, dist_dir, product_name
    )

    if notary_user:
        notarize_package(dmg_path, notary_user, notary_password, notary_team_id)
    else:
        print(
            f"Warning: Skipped notarization, env var {NOTARY_USER_ENV} not set",
            file=sys.stderr,
        )


def package_env_vars():
    codesign_id = env.get_env(CODESIGN_ENV, required=False)
    cert_base64 = env.get_env(CERT_P12_ENV, required=False)
    notary_user = env.get_env(NOTARY_USER_ENV, required=False)

    if notary_user:
        notary_password = env.get_env("APPLE_NOTARY_PASSWORD")
        notary_team_id = env.get_env("APPLE_TEAM_ID")
    else:
        notary_password = None
        notary_team_id = None

    if cert_base64:
        cert_password = env.get_env("APPLE_P12_PASSWORD")
    else:
        cert_password = None

    return (
        codesign_id,
        cert_base64,
        cert_password,
        notary_user,
        notary_password,
        notary_team_id,
    )


def build_bundle(bundle_source_dir):

    # it's important to build a new bundle every time, so that we catch bugs with fresh builds.
    if os.path.exists(bundle_source_dir):
        print(f"Bundle already exists, deleting: {bundle_source_dir}")
        shutil.rmtree(bundle_source_dir)

    print("Building bundle...")

    # cmake build install target should run macdeployqt
    cmd_utils.run("cmake --build build --target install", shell=True, print_cmd=True)


def sign_bundle(bundle_source_dir, codesign_id):
    print(f"Signing bundle {bundle_source_dir}...")

    assert_certificate_installed(codesign_id)
    cmd_utils.run(
        [
            CODESIGN_PATH,
            "-f",
            "--options",
            "runtime",
            "--deep",
            "-s",
            codesign_id,
            bundle_source_dir,
        ]
    )


def assert_certificate_installed(codesign_id):
    print(f"Checking certificate: {codesign_id}")

    installed = cmd_utils.run(
        "security find-identity -v -p codesigning",
        get_output=True,
        shell=True,
        print_cmd=True,
    )

    if codesign_id not in installed.stdout:
        raise RuntimeError("Code signing certificate not installed or has expired")


def build_dmg(bundle_source_dir, filename_base, source_dir, dist_dir, product_name):
    settings_path = (
        SETTINGS_FILE if source_dir is None else os.path.join(source_dir, SETTINGS_FILE)
    )
    settings_path_abs = os.path.abspath(settings_path)
    app_path_abs = os.path.abspath(bundle_source_dir)

    # cwd for dmgbuild, since setting the dmg filename to a path (include the dist dir) seems to
    # make the dmg disappear and never writes to the specified path. the dmgbuild module also
    # creates a temporary file in cwd, so it makes sense to change to the dist dir.
    print(f"Changing directory to: {os.path.abspath(dist_dir)}")
    cwd = os.getcwd()
    os.makedirs(dist_dir, exist_ok=True)
    os.chdir(dist_dir)

    try:
        dmg_filename = f"{filename_base}.dmg"
        dmg_path = os.path.join(dist_dir, dmg_filename)
        print(f"Building package {dmg_path}...")
        dmgbuild.build_dmg(
            dmg_filename,
            product_name,
            settings_file=settings_path_abs,
            defines={
                "app": app_path_abs,
            },
        )
    finally:
        print(f"Changing directory back to: {cwd}")
        os.chdir(cwd)

    return dmg_path


def install_certificate(cert_base64, cert_password):
    if not cert_base64:
        raise ValueError("Certificate base 64 not provided")

    if not cert_password:
        raise ValueError("Certificate password not provided")

    with Certificate(cert_base64, "p12") as cert_path:
        print(f"Installing certificate: {cert_path}")

        # WARNING: contains private key password, never print this command
        cmd_utils.run(
            [
                SUDO_PATH,
                SECURITY_PATH,
                "import",
                cert_path,
                "-k",
                KEYCHAIN_PATH,
                "-P",
                cert_password,
                "-T",
                CODESIGN_PATH,
                "-T",
                SECURITY_PATH,
            ],
        )


def notarize_package(dmg_path, user, password, team_id):
    print(f"Notarizing package {dmg_path}...")
    notary_tool = NotaryTool()
    notary_tool.store_credentials(user, password, team_id)
    notary_tool.submit_and_wait(dmg_path)


def get_xcode_path():
    result = cmd_utils.run(
        [XCODE_SELECT_PATH, "-p"], get_output=True, shell=False, print_cmd=True
    )
    return result.stdout.strip()


class NotaryTool:
    """
    Provides a wrapper around the notarytool command line tool.
    """

    def __init__(self):
        self.xcode_path = get_xcode_path()

    def get_path(self):
        return f"{self.xcode_path}{NOTARYTOOL_PATH}"

    def store_credentials(self, user, password, team_id):
        print("Storing credentials for notary tool...")
        notarytool_path = self.get_path()

        # WARNING: contains password, never print this command
        cmd_utils.run(
            [
                notarytool_path,
                "store-credentials",
                "notarytool-password",
                "--team-id",
                team_id,
                "--apple-id",
                user,
                "--password",
                password,
            ]
        )

    def submit_and_wait(self, dmg_filename):
        print("Submitting notarization request...")
        submit_result = self.run_submit_command(dmg_filename)
        request_id = submit_result["id"]

        print(f"Notary submitted, waiting for request: {request_id}")
        start = time.time()
        wait_result = self.run_wait_command(request_id)
        status = wait_result["status"]

        time_taken = time.time() - start
        print(f"Notary complete in {time_taken:.2f}s, status: {status}")
        if status == "Accepted":
            print("Notarization successful.")
        elif status == "Invalid" or status == "Rejected":
            raise ValueError(f"Notarization failed, status: {status}")
        else:
            raise ValueError(f"Unknown status: {status}")

    def run_submit_command(self, dmg_filename):
        if not os.path.exists(dmg_filename):
            raise FileNotFoundError(f"File not found: {dmg_filename}")

        result = cmd_utils.run(
            [
                self.get_path(),
                "submit",
                dmg_filename,
                "--keychain-profile",
                "notarytool-password",
                "--output-format",
                "json",
            ],
            get_output=True,
            shell=False,
            print_cmd=True,
        )

        if result.stderr:
            return json.loads(result.stderr)
        else:
            return json.loads(result.stdout)

    def run_wait_command(self, request_id):
        result = cmd_utils.run(
            [
                self.get_path(),
                "wait",
                request_id,
                "--keychain-profile",
                "notarytool-password",
                "--output-format",
                "json",
            ],
            get_output=True,
            shell=False,
            print_cmd=True,
        )

        if result.stderr:
            return json.loads(result.stderr)
        else:
            return json.loads(result.stdout)
