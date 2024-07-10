import dmgbuild  # type: ignore
import os, time, json
import lib.cmd_utils as cmd_utils
import lib.env as env
from lib.certificate import Certificate

cmake_env_var = "CMAKE_PREFIX_PATH"
shell_rc = "~/.zshrc"
dist_dir = "dist"
product_name = "Synergy 1"
settings_file = "res/dist/macos/dmgbuild/settings.py"
app_path = "build/bundle/Synergy.app"
security_path = "/usr/bin/security"
sudo_path = "/usr/bin/sudo"
notarytool_path = "/usr/bin/notarytool"
codesign_path = "/usr/bin/codesign"
xcode_select_path = "/usr/bin/xcode-select"
keychain_path = "/Library/Keychains/System.keychain"


def set_env_var(name, value):
    text = f'export {name}="${name}:{value}"'
    file = os.path.expanduser(shell_rc)
    with open(file, "r") as f:
        if text in f.read():
            return

    print(f"Setting environment variable: {name}={name}")
    with open(file, "a") as f:
        f.write(f"\n{text}")
        print(f"Appended to {shell_rc}: {text}")


def set_cmake_prefix_env_var(cmake_prefix_command):
    result = cmd_utils.run(
        cmake_prefix_command, get_output=True, shell=True, print_cmd=True
    )
    cmake_prefix = result.stdout.strip()
    set_env_var(cmake_env_var, cmake_prefix)


def package(filename_base):
    """
    Package the application for macOS.
    The app bundle must be signed, or an error will occur:
    > EXC_BAD_ACCESS (SIGKILL (Code Signature Invalid))
    """

    codesign_id = env.get_env("APPLE_CODESIGN_ID")
    cert_base64 = env.get_env("APPLE_P12_CERTIFICATE")
    cert_password = env.get_env("APPLE_P12_PASSWORD")

    build_bundle()
    install_certificate(cert_base64, cert_password)
    assert_certificate_installed(codesign_id)
    sign_bundle(codesign_id)
    dmg_path = build_dmg(filename_base)
    notarize_package(dmg_path)


def build_bundle():
    print("Building bundle...")
    # cmake build install target should run macdeployqt
    cmd_utils.run("cmake --build build --target install", shell=True, print_cmd=True)


def sign_bundle(codesign_id):
    print(f"Signing bundle {app_path}...")
    cmd_utils.run(
        [
            codesign_path,
            "-f",
            "--options",
            "runtime",
            "--deep",
            "-s",
            codesign_id,
            app_path,
        ]
    )


def assert_certificate_installed(codesign_id):
    installed = cmd_utils.run(
        "security find-identity -v -p codesigning",
        get_output=True,
        shell=True,
        print_cmd=True,
    )

    if codesign_id not in installed.stdout:
        raise RuntimeError("Code signing certificate not installed or has expired")


def build_dmg(filename_base):
    settings_file_abs = os.path.abspath(settings_file)
    app_path_abs = os.path.abspath(app_path)

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
            settings_file=settings_file_abs,
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
                sudo_path,
                security_path,
                "import",
                cert_path,
                "-k",
                keychain_path,
                "-P",
                cert_password,
                "-T",
                codesign_path,
                "-T",
                security_path,
            ],
        )


def notarize_package(dmg_path):
    print(f"Notarizing package {dmg_path}...")
    notary_tool = NotaryTool()
    notary_tool.store_credentials(
        env.get_env("APPLE_NOTARY_USER"),
        env.get_env("APPLE_NOTARY_PASSWORD"),
        env.get_env("APPLE_TEAM_ID"),
    )

    notary_tool.submit_and_wait(dmg_path)


def get_xcode_path():
    result = cmd_utils.run(
        [xcode_select_path, "-p"], get_output=True, shell=False, print_cmd=True
    )
    return result.stdout.strip()


class NotaryTool:
    """
    Provides a wrapper around the notarytool command line tool.
    """

    def __init__(self):
        self.xcode_path = get_xcode_path()

    def get_path(self):
        return f"{self.xcode_path}{notarytool_path}"

    def store_credentials(self, user, password, team_id):
        print("Storing credentials for notary tool...")
        notarytool_path = self.get_path()

        # WARNING: contains password, never print this command
        cmd_utils.run(
            [
                notarytool_path,
                "store-credentials",
                "notarytool-password",
                "--apple-id",
                user,
                "--team-id",
                team_id,
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
