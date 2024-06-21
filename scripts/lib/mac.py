import os, subprocess, base64, time, json, shutil, sys
from lib import cmd_utils, env

cmake_env_var = "CMAKE_PREFIX_PATH"
shell_rc = "~/.zshrc"
cert_path = "tmp/codesign.p12"
dmg_path = "dist/synergy-macos.dmg"
product_name = "Synergy"
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
    result = cmd_utils.run(cmake_prefix_command, get_output=True)
    cmake_prefix = result.stdout.strip()
    set_env_var(cmake_env_var, cmake_prefix)


def package(config):
    if os.path.exists(app_path):
        print("Deleting existing bundle")
        shutil.rmtree(app_path)

    # qt_prefix_command = config.get_os_value("qt-prefix-command")
    # qt_prefix_path = cmd_utils.run(qt_prefix_command, get_output=True).stdout.strip()

    # # is this necessary? can cmake find qt tools on it's own?
    # #   export PATH="$(brew --prefix qt5)/bin:$PATH"
    # ci_env = env.is_running_in_ci()
    # if not ci_env:
    #     set_env_var("PATH", f"{qt_prefix_path}/bin")

    # cmake runs macdeployqt
    print("Building bundle...")
    cmd_utils.run("cmake --build build --target install")

    install_certificate(
        env.get_env_var("APPLE_P12_CERTIFICATE"),
        env.get_env_var("APPLE_P12_PASSWORD"),
    )

    codesign_id = env.get_env_var("APPLE_CODESIGN_ID")
    is_certificate_installed(codesign_id)

    print(f"Signing bundle {app_path}...")
    sys.stdout.flush()
    subprocess.run(
        [
            codesign_path,
            "-f",
            "--options",
            "runtime",
            "--deep",
            "-s",
            codesign_id,
            app_path,
        ],
        check=True,
    )

    build_dmg()

    print(f"Notarizing package {dmg_path}...")
    notarize_package()


def is_certificate_installed(codesign_id):
    installed = cmd_utils.run(
        "security find-identity -v -p codesigning", get_output=True
    )

    if codesign_id not in installed.stdout:
        raise RuntimeError("Code signing certificate not installed or has expired")


def build_dmg():
    env.ensure_module("dmgbuild", "dmgbuild")
    import dmgbuild  # type: ignore

    settings_file_abs = os.path.abspath(settings_file)
    app_path_abs = os.path.abspath(app_path)

    # cwd for dmgbuild, since setting the dmg filename to a path (include the dist dir) seems to
    # make the dmg disappear and never writes to the specified path. the dmgbuild module also
    # creates a temporary file in cwd, so it makes sense to change to the dist dir.
    dist_dir = os.path.dirname(dmg_path)
    print(f"Changing directory to: {os.path.abspath(dist_dir)}")
    cwd = os.getcwd()
    os.makedirs(dist_dir, exist_ok=True)
    os.chdir(dist_dir)

    try:
        print(f"Building package {dmg_path}...")
        dmg_filename = os.path.basename(dmg_path)
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


def install_certificate(cert_base64, cert_password):
    if not cert_base64:
        raise ValueError("Certificate base 64 not provided")

    if not cert_password:
        raise ValueError("Certificate password not provided")

    print(f"Decoding certificate to: {cert_path}")
    cert_bytes = base64.b64decode(cert_base64)
    os.makedirs(os.path.dirname(cert_path), exist_ok=True)
    with open(cert_path, "wb") as cert_file:
        cert_file.write(cert_bytes)

    print(f"Installing certificate: {cert_path}")
    sys.stdout.flush()

    try:
        # warning: contains private key password, never print this command
        subprocess.run(
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
            check=True,
        )
    except subprocess.CalledProcessError as e:
        # important: suppress the original args with `from None` to avoid leaking the password
        raise subprocess.CalledProcessError(e.returncode, security_path) from None
    except Exception as e:
        # important: suppress the original args with `from None` to avoid leaking the password
        raise RuntimeError(f"Command failed: {security_path}") from None
    finally:
        # not strictly necessary for ci, but when run on a dev machine, it reduces the risk
        # that private keys are left on the filesystem
        print(f"Removing temporary certificate file: {cert_path}")
        os.remove(cert_path)


def notarize_package():
    notary_tool = NotaryTool()
    notary_tool.store_credentials(
        env.get_env_var("APPLE_NOTARY_USER"),
        env.get_env_var("APPLE_NOTARY_PASSWORD"),
        env.get_env_var("APPLE_TEAM_ID"),
    )

    notary_tool.submit_and_wait(dmg_path)


def get_xcode_path():
    result = cmd_utils.run([xcode_select_path, "-p"], get_output=True, shell=False)
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
        sys.stdout.flush()

        notarytool_path = self.get_path()
        try:
            # warning: contains password, never print this command
            subprocess.run(
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
                ],
                check=True,
            )
        except subprocess.CalledProcessError as e:
            # important: suppress the original args with `from None` to avoid leaking the password
            raise subprocess.CalledProcessError(e.returncode, notarytool_path) from None
        except Exception as e:
            # important: suppress the original args with `from None` to avoid leaking the password
            raise RuntimeError(f"Command failed: {notarytool_path}") from None

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
