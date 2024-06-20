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
    store_notary_tool_password(
        env.get_env_var("APPLE_NOTARY_USER"),
        env.get_env_var("APPLE_NOTARY_PASSWORD"),
        env.get_env_var("APPLE_TEAM_ID"),
    )

    submit_notary_tool_request()


def submit_notary_tool_request():
    print("Submitting notarization request...")
    xcode_path = get_xcode_path()
    result = cmd_utils.run(
        [
            f"{xcode_path}/usr/bin/notarytool",
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

    submit_output = json.loads(result.stdout)
    request_id = submit_output["id"]
    print(f"Request ID: {request_id}")

    print("Waiting for notarization...")
    while True:
        time.sleep(10)
        result = cmd_utils.run(
            [
                f"{xcode_path}/usr/bin/notarytool",
                "log",
                request_id,
                "--keychain-profile",
                "notarytool-password",
                "--output-format",
                "json",
                "notarytool.json",
            ],
            shell=False,
            check=False,
            get_output=True,
            print_cmd=False,
        )

        if result.returncode == 69:
            print("Notarization not started...")
            log_output = json.loads(result.stderr)
            message = log_output["message"] if "message" in log_output else "Unknown"
            print(f"Message: {message}")
            continue

        log = json.load(open("notarytool.json"))
        status = log["status"] if "status" in log else "Unknown"
        print(f"Notarization status: {status}")

        if status == "Success":
            print("Notarization successful.")
            break
        elif status == "Invalid":
            raise ValueError("Notarization failed.")
        elif status == "Accepted":
            continue
        else:
            raise ValueError(f"Unknown status: {status}")


def get_xcode_path():
    result = cmd_utils.run(
        ["/usr/bin/xcode-select", "-p"], get_output=True, shell=False
    )
    return result.stdout.strip()


def store_notary_tool_password(user, password, team_id):
    print("Storing notary tool password...")
    xcode_path = get_xcode_path()
    subprocess.run(
        [
            f"{xcode_path}/usr/bin/notarytool",
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
