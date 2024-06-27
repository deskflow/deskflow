import os, base64

temp_path = "tmp/certificate"


class Certificate:
    """
    Installs a certificate from a base64 string, and returns the path to the certificate.
    Once the context is exited, the certificate is removed from the filesystem.

    Example usage:
    with Certificate(base64) as cert_path:
        print(f"Certificate path: {cert_path}")
    """

    def __init__(self, base64, file_ext):
        self.base64 = base64
        self.temp_filename = f"{temp_path}.{file_ext}"

    def __enter__(self):
        print(f"Decoding certificate to temporary path: {self.temp_filename}")
        try:
            cert_bytes = base64.b64decode(self.base64)
        except Exception as e:
            raise ValueError("Failed to decode certificate base64") from e

        os.makedirs(os.path.dirname(self.temp_filename), exist_ok=True)
        with open(self.temp_filename, "wb") as cert_file:
            cert_file.write(cert_bytes)

        return self.temp_filename

    def __exit__(self, _exc_type, _exc_value, _traceback):
        # not strictly necessary for ci, but when run on a dev machine, it reduces the risk
        # that private keys are left on the filesystem
        print(f"Removing temporary certificate file: {self.temp_filename}")
        os.remove(self.temp_filename)

        # propagate exceptions
        return False
