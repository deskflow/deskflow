import hashlib


def generate_sha256_file(filename):
    generate_checksum_file(filename, hashlib.sha256(), "sha256")


def generate_checksum_file(filename, checksum, extension):
    with open(filename, "rb") as f:
        while chunk := f.read(4096):
            checksum.update(chunk)

    with open(f"{filename}.{extension}", "w") as f:
        f.write(checksum.hexdigest())
