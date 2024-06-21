import os, subprocess, base64, time, json
from tempfile import NamedTemporaryFile
from lib import cmd_utils, env

cmake_env_var = "CMAKE_PREFIX_PATH"
shell_rc = "~/.zshrc"
cert_path = "/tmp/certificate.p12"
dmg_filename = "Synergy.dmg"
product_name = "Synergy"


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
    result = cmd_utils.run(cmake_prefix_command, get_output=True)
    cmake_prefix = result.stdout.strip()

    set_env_var(cmake_env_var, cmake_prefix)


def package():
    env.ensure_module("dmgbuild", "dmgbuild")
    import dmgbuild

    install_certificate(
        env.get_env_var("APPLE_P12_CERTIFICATE"),
        env.get_env_var("APPLE_P12_PASSWORD"),
    )

    print(f"Building package {dmg_filename}...")
    dmgbuild.build_dmg(
        product_name,
        dmg_filename,
        settings_file="dist/macos/dmgbuild/settings.py",
    )

    print(f"Notarizing package {dmg_filename}...")
    notarize_package()


def install_certificate(cert_base64, cert_password):
    if not cert_base64:
        raise ValueError("Certificate base 64 not provided")

    if not cert_password:
        raise ValueError("Certificate password not provided")

    print(f"Decoding certificate to: {cert_path}")
    cert_bytes = base64.b64decode(cert_base64)
    with open(cert_path, "wb") as cert_file:
        cert_file.write(cert_bytes)

    print(f"Installing certificate: {cert_path}")

    # Warning: Contains private key password, never print this command
    subprocess.run(
        [
            "/usr/bin/sudo",
            "/usr/bin/security",
            "import",
            cert_path,
            "-k",
            "/Library/Keychains/System.keychain",
            "-P",
            cert_password,
            "-T",
            "/usr/bin/codesign",
            "-T",
            "/usr/bin/security",
        ],
        check=True,
    )
    print("Certificate installed successfully.")


def notarize_package():
    notary_tool = NotaryTool()
    notary_tool.store_credentials(
        env.get_env_var("APPLE_NOTARY_USER"),
        env.get_env_var("APPLE_NOTARY_PASSWORD"),
        env.get_env_var("APPLE_TEAM_ID"),
    )

    notary_tool.submit_and_wait(dmg_filename)


def get_xcode_path():
    result = cmd_utils.run(
        ["/usr/bin/xcode-select", "-p"], get_output=True, shell=False
    )
    return result.stdout.strip()


class NotaryTool:
    """
    Provides a wrapper around the notarytool command line tool.
    """

    def __init__(self):
        self.xcode_path = get_xcode_path()

    def get_path(self):
        return f"{self.xcode_path}/usr/bin/notarytool"

    def store_credentials(self, user, password, team_id):
        print("Storing credentials for notary tool...")

        # Warning: Contains password, never print this command
        subprocess.run(
            [
                self.get_path(),
                "store-credentials",
                "notarytool-password",
                "--apple-id",
                user,
                "--team-id",
                team_id,
                "--password",
                password,
            ],
            check=True,
        )

    def run_submit_command(self, dmg_filename):
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
        )

        if result.stderr:
            return json.loads(result.stderr)
        else:
            return json.loads(result.stdout)

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
