import os, subprocess, base64
from lib import cmd_utils

cmake_env_var = "CMAKE_PREFIX_PATH"
shell_rc = "~/.zshrc"
cert_path = "/tmp/certificate.p12"


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
    result = cmd_utils.run(cmake_prefix_command, get_stdout=True)
    cmake_prefix = result.stdout.strip()

    set_env_var(cmake_env_var, cmake_prefix)


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
    )
    print("Certificate installed successfully.")
