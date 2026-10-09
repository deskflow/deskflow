# SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
# SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
"""Run on macOS: python3 handoff_ssh_smoke.py /path/to/deskflow-handoff.

Uses a temporary loopback sshd and disposable keys, without changing Remote Login
or ~/.ssh. Creates uniquely named test files in Downloads and removes only those
verified copies. An optional second argument is an installed app bundle identifier;
that check opens a disposable text document in the specified app.
"""

import getpass
import json
import pathlib
import shlex
import socket
import subprocess
import sys
import tempfile
import time
import uuid


def run(*args, **kwargs):
    return subprocess.run(args, check=True, capture_output=True, **kwargs)


def main():
    receiver = pathlib.Path(sys.argv[1]).resolve(strict=True)
    token = uuid.uuid4().hex
    filename = f"deskflow-smoke-{token}.txt"
    downloads = pathlib.Path.home() / "Downloads"
    with tempfile.TemporaryDirectory(prefix="deskflow-ssh-") as temporary:
        root = pathlib.Path(temporary)
        for name in ("client", "host"):
            run(
                "/usr/bin/ssh-keygen",
                "-q",
                "-t",
                "ed25519",
                "-N",
                "",
                "-f",
                str(root / name),
            )
        (root / "authorized_keys").write_bytes((root / "client.pub").read_bytes())
        (root / "authorized_keys").chmod(0o600)
        with socket.socket() as sock:
            sock.bind(("127.0.0.1", 0))
            port = sock.getsockname()[1]
        (root / "known_hosts").write_text(
            f"[127.0.0.1]:{port} " + (root / "host.pub").read_text()
        )
        (root / "sshd_config").write_text(f"""Port {port}
ListenAddress 127.0.0.1
HostKey {root}/host
PidFile {root}/pid
AuthorizedKeysFile {root}/authorized_keys
PasswordAuthentication no
KbdInteractiveAuthentication no
UsePAM no
AllowUsers {getpass.getuser()}
AllowTcpForwarding no
PermitTTY no
ForceCommand {shlex.quote(str(receiver))}
""")
        ssh = [
            "/usr/bin/ssh",
            "-T",
            "-p",
            str(port),
            "-i",
            str(root / "client"),
            "-oBatchMode=yes",
            "-oStrictHostKeyChecking=yes",
            "-oIdentitiesOnly=yes",
            f"-oUserKnownHostsFile={root}/known_hosts",
            "127.0.0.1",
            "'/Applications/Deskflow.app/Contents/MacOS/deskflow-handoff'",
        ]

        def packet(data=None, application="", complete=True):
            header = json.dumps(
                {
                    "version": 1,
                    "files": []
                    if data is None
                    else [{"name": filename, "size": str(len(data))}],
                    "application": application,
                    "url": "",
                },
                separators=(",", ":"),
            ).encode()
            return (
                str(len(header)).encode()
                + b"\n"
                + header
                + (data or b"")
                + (b"DONE" if complete else b"")
            )

        def transfer(data):
            return subprocess.run(
                ssh, input=data, capture_output=True, timeout=40, check=False
            )

        with (root / "log").open("wb") as log:
            server = subprocess.Popen(
                ["/usr/sbin/sshd", "-D", "-e", "-f", str(root / "sshd_config")],
                stdout=log,
                stderr=log,
            )
            try:
                for _ in range(50):
                    if server.poll() is not None:
                        raise RuntimeError((root / "log").read_text())
                    try:
                        with socket.create_connection(("127.0.0.1", port), timeout=0.1):
                            break
                    except OSError:
                        time.sleep(0.1)
                data = b"Deskflow SSH test\x00\xff" * 20000
                result = transfer(packet(data))
                assert result.returncode == 0, result.stderr
                copies = list(downloads.glob(f"deskflow-*/{filename}"))
                assert len(copies) == 1, copies
                assert copies[0].read_bytes() == data
                print("PASS: SSH file transfer, real receiver, exact bytes")

                result = transfer(packet(b"incomplete", complete=False))
                assert result.returncode != 0
                assert not list(downloads.glob(f".deskflow-*/{filename}"))
                print("PASS: unfinished transfer is rejected and staging is removed")

                interrupted = subprocess.Popen(
                    ssh,
                    stdin=subprocess.PIPE,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                )
                try:
                    interrupted.stdin.write(packet(data)[:65536])
                    interrupted.stdin.flush()
                    for _ in range(50):
                        if list(downloads.glob(f".deskflow-*/{filename}")):
                            break
                        time.sleep(0.1)
                    assert list(downloads.glob(f".deskflow-*/{filename}")), (
                        "Receiver never started"
                    )
                    interrupted.kill()
                    interrupted.wait(timeout=5)
                    for _ in range(50):
                        if not list(downloads.glob(f".deskflow-*/{filename}")):
                            break
                        time.sleep(0.1)
                    assert not list(downloads.glob(f".deskflow-*/{filename}"))
                    print("PASS: killed SSH sender leaves no partial copy")
                finally:
                    if interrupted.poll() is None:
                        interrupted.kill()
                        interrupted.wait(timeout=5)
                    interrupted.stdin.close()
                    interrupted.stdout.close()
                    interrupted.stderr.close()

                result = transfer(
                    packet(b"saved", application="org.deskflow.not.installed")
                )
                assert result.returncode != 0 and b"not installed" in result.stderr, (
                    result.stderr
                )
                assert any(
                    path.read_bytes() == b"saved"
                    for path in downloads.glob(f"deskflow-*/{filename}")
                )
                print("PASS: missing app reports failure and preserves completed copy")

                if len(sys.argv) > 2:
                    result = transfer(
                        packet(
                            b"Deskflow native document handoff test\n",
                            application=sys.argv[2],
                        )
                    )
                    assert result.returncode == 0, result.stderr
                    print(
                        f"PASS: native document opening in {sys.argv[2]} ({filename})"
                    )

                # A different host key must fail before any transfer can start.
                (root / "known_hosts").write_text(
                    f"[127.0.0.1]:{port} " + (root / "client.pub").read_text()
                )
                result = transfer(packet(b"should not arrive"))
                assert (
                    result.returncode != 0
                    and b"HOST IDENTIFICATION HAS CHANGED" in result.stderr
                )
                print("PASS: SSH rejects a changed host key")
            finally:
                server.terminate()
                server.wait(timeout=5)
                for path in downloads.glob(f"deskflow-*/{filename}"):
                    if path.read_bytes() in (
                        data,
                        b"saved",
                        b"Deskflow native document handoff test\n",
                    ):
                        path.unlink()
                        path.parent.rmdir()


if __name__ == "__main__":
    main()
