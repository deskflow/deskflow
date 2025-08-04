#!/usr/bin/env python3

# Deskflow -- mouse and keyboard sharing utility
# SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
# SPDX-FileCopyrightText: 2025 Red Hat

# This is a protocol debugger working as a MITM process. Typically you would
# run it on the same host as the Deskflow server, then connect a client to it
# on the default port (24801). All messages between server and client are logged.

import argparse
import binascii
import logging
import os
import select
import socket
import struct
import sys
from dataclasses import dataclass, field, fields
from typing import Optional, Self, Tuple

default_log_format = "%(asctime)s - %(name)-15s - %(levelname).1s - %(message)s"

# Create loggers for client and server
logger = logging.getLogger("deskflow")
clogger = logging.getLogger("server ← client")
slogger = logging.getLogger("server → client")

use_color = not os.getenv("NO_COLORS") and (
    os.getenv("FORCE_COLORS") or sys.stdout.isatty()
)

if use_color:
    try:
        import colorlog

        # Create separate handlers with different colors for each logger
        deskflow_handler = colorlog.StreamHandler()
        deskflow_handler.setFormatter(
            colorlog.ColoredFormatter(
                "%(green)s%(asctime)s - %(name)-15s - %(levelname).1s - %(message)s%(reset)s",
                datefmt="%H:%M:%S",
            )
        )
        client_handler = colorlog.StreamHandler()
        client_handler.setFormatter(
            colorlog.ColoredFormatter(
                "%(blue)s%(asctime)s - %(name)-15s - %(levelname).1s - %(message)s%(reset)s",
                datefmt="%H:%M:%S",
            )
        )
        server_handler = colorlog.StreamHandler()
        server_handler.setFormatter(
            colorlog.ColoredFormatter(
                "%(purple)s%(asctime)s - %(name)-15s - %(levelname).1s - %(message)s%(reset)s",
                datefmt="%H:%M:%S",
            )
        )

        logger.addHandler(deskflow_handler)
        clogger.addHandler(client_handler)
        slogger.addHandler(server_handler)

        # Remove the default handler that might have been added
        logger.propagate = False
        clogger.propagate = False
        slogger.propagate = False

    except ImportError:
        use_color = False

if not use_color:
    # Configure logging
    logging.basicConfig(
        level=logging.DEBUG,
        format=default_log_format,
        datefmt="%H:%M:%S",
    )


# Protocol constants
DEFAULT_PORT = 24800
BUFFER_SIZE = 4096  # 4KB buffer for reading

show_keycodes = False


@dataclass
class ProtocolMessage:
    """Base class for all protocol messages."""

    # Note that the format string is for human verification
    # only, it is not used for parsing. This hopefully
    # exposes any oddities in the protocol where the parsing
    # string mismatches what one would expect.
    format_string: str = None  # type: ignore
    code: str = None  # type: ignore

    @property
    def is_obfuscated(self) -> bool:
        return False

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        return cls()

    def __str__(self) -> str:
        exclude = ["format_string", "code"]
        fs = [
            f"{f.name}={getattr(self, f.name)}"
            for f in fields(self)
            if not f.name.startswith("_") and f.name not in exclude
        ]
        return f"{self.code} {', '.join(fs)}{' (obfuscated)' if self.is_obfuscated else ''}"


# Hello messages don't have a common prefix so for convenience
# we just implement them as separate classes
@dataclass
class _MessageHello(ProtocolMessage):
    major: int = 0
    minor: int = 0

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        versions = data[len(cls.code) :]
        return cls(
            major=int.from_bytes(versions[0:2], byteorder="big"),
            minor=int.from_bytes(versions[2:4], byteorder="big"),
        )


@dataclass
class MessageHelloBarrier(_MessageHello):
    code: str = "Barrier"


@dataclass
class MessageHelloSynergy(_MessageHello):
    code: str = "Synergy"


@dataclass
class MessageCNOP(ProtocolMessage):
    code: str = "CNOP"
    format_string: str = "CNOP"


@dataclass
class MessageCBYE(ProtocolMessage):
    code: str = "CBYE"
    format_string: str = ""


@dataclass
class MessageCINN(ProtocolMessage):
    code: str = "CINN"
    format_string: str = "CINN%2i%2i%4i%2i"
    x: int = 0
    y: int = 0
    sequence: int = 0
    mask: int = 0

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        data = data[len(cls.code) :]
        return cls(
            x=int.from_bytes(data[0:2], byteorder="big"),
            y=int.from_bytes(data[2:4], byteorder="big"),
            sequence=int.from_bytes(data[4:8], byteorder="big"),
            mask=int.from_bytes(data[8:10], byteorder="big"),
        )


@dataclass
class MessageCOUT(ProtocolMessage):
    code: str = "COUT"
    format_string: str = "COUT"


@dataclass
class MessageCCLP(ProtocolMessage):
    code: str = "CCLP"
    format_string: str = "CCLP%1i%4i"
    id: int = 0
    sequence: int = 0

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        data = data[len(cls.code) :]
        return cls(
            id=int.from_bytes(data[0:1], byteorder="big"),
            sequence=int.from_bytes(data[1:5], byteorder="big"),
        )


@dataclass
class MessageCSEC(ProtocolMessage):
    code: str = "CSEC"
    format_string: str = "CSEC%1i"
    state: int = 0

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        data = data[len(cls.code) :]
        return cls(
            state=int.from_bytes(data[0:1], byteorder="big"),
        )


@dataclass
class MessageCROP(ProtocolMessage):
    code: str = "CROP"
    format_string: str = "CROP"


@dataclass
class MessageCIAK(ProtocolMessage):
    code: str = "CIAK"
    format_string: str = "CIAK"


@dataclass
class MessageCALV(ProtocolMessage):
    code: str = "CALV"
    format_string: str = "CALV"


@dataclass
class MessageDKDL(ProtocolMessage):
    code: str = "DKDL"
    format_string: str = "DKDL%2i%2i%2i%s"
    keyid: int = 0
    mask: int = 0
    button: int = 0
    lang: str = ""

    @property
    def is_obfuscated(self) -> bool:
        return not show_keycodes

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        data = data[len(cls.code) :]
        if show_keycodes:
            keyid = int.from_bytes(data[0:2], byteorder="big")
            button = int.from_bytes(data[4:6], byteorder="big")
        else:
            keyid = 97
            button = 38
        return cls(
            keyid=keyid,
            mask=int.from_bytes(data[2:4], byteorder="big"),
            button=button,
            lang=data[6:].decode("utf-8", errors="replace"),
        )


@dataclass
class MessageDKDN(ProtocolMessage):
    code: str = "DKDN"
    format_string: str = "DKDN%2i%2i%2i"
    keyid: int = 0
    mask: int = 0
    button: int = 0

    @property
    def is_obfuscated(self) -> bool:
        return not show_keycodes

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        data = data[len(cls.code) :]
        if show_keycodes:
            keyid = int.from_bytes(data[0:2], byteorder="big")
            button = int.from_bytes(data[4:6], byteorder="big")
        else:
            keyid = 97
            button = 38

        return cls(
            keyid=keyid,
            mask=int.from_bytes(data[2:4], byteorder="big"),
            button=button,
        )


@dataclass
class MessageDKRP(ProtocolMessage):
    code: str = "DKRP"
    format_string: str = "DKRP%2i%2i%2i%2i%s"
    keyid: int = 0
    mask: int = 0
    button: int = 0
    count: int = 0
    lang: str = ""

    @property
    def is_obfuscated(self) -> bool:
        return not show_keycodes

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        data = data[len(cls.code) :]
        if show_keycodes:
            keyid = int.from_bytes(data[0:2], byteorder="big")
            button = int.from_bytes(data[4:6], byteorder="big")
        else:
            keyid = 97
            button = 38

        return cls(
            keyid=keyid,
            mask=int.from_bytes(data[2:4], byteorder="big"),
            button=button,
            count=int.from_bytes(data[6:8], byteorder="big"),
            lang=data[8:].decode("utf-8", errors="replace"),
        )


@dataclass
class MessageDKUP(ProtocolMessage):
    code: str = "DKUP"
    format_string: str = "DKUP%2i%2i%2i"
    keyid: int = 0
    mask: int = 0
    button: int = 0

    @property
    def is_obfuscated(self) -> bool:
        return not show_keycodes

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        data = data[len(cls.code) :]
        if show_keycodes:
            keyid = int.from_bytes(data[0:2], byteorder="big")
            button = int.from_bytes(data[2:4], byteorder="big")
        else:
            keyid = 97
            button = 38

        return cls(
            keyid=keyid,
            mask=int.from_bytes(data[2:4], byteorder="big"),
            button=button,
        )


@dataclass
class MessageDMDN(ProtocolMessage):
    code: str = "DMDN"
    format_string: str = "DMDN%1i"
    button: int = 0

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        data = data[len(cls.code) :]
        return cls(
            button=int.from_bytes(data[0:1], byteorder="big"),
        )


@dataclass
class MessageDMUP(ProtocolMessage):
    code: str = "DMUP"
    format_string: str = "DMUP%1i"
    button: int = 0

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        data = data[len(cls.code) :]
        return cls(
            button=int.from_bytes(data[0:1], byteorder="big"),
        )


@dataclass
class MessageDMMV(ProtocolMessage):
    code: str = "DMMV"
    format_string: str = "DMMV%2i%2i"
    x: int = 0
    y: int = 0

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        data = data[len(cls.code) :]
        return cls(
            x=int.from_bytes(data[0:2], byteorder="big"),
            y=int.from_bytes(data[2:4], byteorder="big"),
        )


@dataclass
class MessageDMRM(ProtocolMessage):
    code: str = "DMRM"
    format_string: str = "DMRM%2i%2i"
    x: int = 0
    y: int = 0

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        data = data[len(cls.code) :]
        return cls(
            x=int.from_bytes(data[0:2], byteorder="big"),
            y=int.from_bytes(data[2:4], byteorder="big"),
        )


@dataclass
class MessageDMWM(ProtocolMessage):
    code: str = "DMWM"
    format_string: str = "DMWM%2i%2i"
    xdelta: int = 0
    ydelta: int = 0

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        data = data[len(cls.code) :]
        return cls(
            xdelta=int.from_bytes(data[0:2], byteorder="big"),
            ydelta=int.from_bytes(data[2:4], byteorder="big"),
        )


@dataclass
class MessageDCLP(ProtocolMessage):
    code: str = "DCLP"
    format_string: str = "DCLP%1i%4i%1i%s"
    id: int = 0
    sequence: int = 0
    mark: int = 0
    data: str = ""

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        data = data[len(cls.code) :]
        return cls(
            id=int.from_bytes(data[0:1], byteorder="big"),
            sequence=int.from_bytes(data[1:5], byteorder="big"),
            mark=int.from_bytes(data[5:6], byteorder="big"),
            data=data[6:].decode("utf-8", errors="replace"),
        )


@dataclass
class MessageDINF(ProtocolMessage):
    code: str = "DINF"
    format_string: str = "DINF%2i%2i%2i%2i%2i%2i%2i"
    x: int = 0
    y: int = 0
    w: int = 0
    h: int = 0
    mx: int = 0
    my: int = 0
    size: int = 0

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        data = data[len(cls.code) :]
        return cls(
            x=int.from_bytes(data[0:2], byteorder="big"),
            y=int.from_bytes(data[2:4], byteorder="big"),
            w=int.from_bytes(data[4:6], byteorder="big"),
            h=int.from_bytes(data[6:8], byteorder="big"),
            mx=int.from_bytes(data[8:10], byteorder="big"),
            my=int.from_bytes(data[10:12], byteorder="big"),
            size=int.from_bytes(data[12:14], byteorder="big"),
        )


@dataclass
class MessageDSOP(ProtocolMessage):
    code: str = "DSOP"
    format_string: str = "DSOP%4I"
    options: int = 0

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        data = data[len(cls.code) :]
        return cls(
            options=int.from_bytes(data[0:4], byteorder="big"),
        )


@dataclass
class MessageDFTR(ProtocolMessage):
    code: str = "DFTR"
    format_string: str = "DFTR%1i%s"
    mark: int = 0
    data: str = ""

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        data = data[len(cls.code) :]
        return cls(
            mark=int.from_bytes(data[0:1], byteorder="big"),
            data=data[1:].decode("utf-8", errors="replace"),
        )


@dataclass
class MessageDDRG(ProtocolMessage):
    code: str = "DDRG"
    format_string: str = "DDRG%2i%s"
    size: int = 0
    data: str = ""

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        data = data[len(cls.code) :]
        return cls(
            size=int.from_bytes(data[0:2], byteorder="big"),
            data=data[2:].decode("utf-8", errors="replace"),
        )


@dataclass
class MessageSECN(ProtocolMessage):
    code: str = "SECN"
    format_string: str = "SECN%s"
    data: str = ""

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        data = data[len(cls.code) :]
        return cls(
            data=data.decode("utf-8", errors="replace"),
        )


@dataclass
class MessageLSYN(ProtocolMessage):
    code: str = "LSYN"
    format_string: str = "LSYN%s"
    data: str = ""

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        data = data[len(cls.code) :]
        return cls(
            data=data.decode("utf-8", errors="replace"),
        )


@dataclass
class MessageQINF(ProtocolMessage):
    code: str = "QINF"
    format_string: str = "QINF"


@dataclass
class MessageEICV(ProtocolMessage):
    code: str = "EICV"
    format_string: str = "EICV%2i%2i"
    major_remote: int = 0
    minor_remote: int = 0

    @classmethod
    def from_bytes(cls, data: bytes) -> Self:
        data = data[len(cls.code) :]
        return cls(
            major_remote=int.from_bytes(data[0:2], byteorder="big"),
            minor_remote=int.from_bytes(data[2:4], byteorder="big"),
        )


@dataclass
class MessageEBSY(ProtocolMessage):
    code: str = "EBSY"
    format_string: str = "EBSY"


@dataclass
class MessageEUNK(ProtocolMessage):
    code: str = "EUNK"
    format_string: str = "EUNK"


@dataclass
class MessageEBAD(ProtocolMessage):
    code: str = "EBAD"
    format_string: str = "EBAD"


def find_local_classes(prefix) -> list[type[ProtocolMessage]]:
    import inspect

    current_module = sys.modules[__name__]

    return [
        c
        for _, c in inspect.getmembers(
            current_module,
            lambda x: inspect.isclass(x)
            and x.__module__ == __name__
            and x.__name__.startswith(prefix),
        )
    ]


MESSAGES = {m.code: m for m in find_local_classes("Message")}


@dataclass
class Message:
    """A protocol message with its length prefix removed."""

    length: int
    data: bytes

    @property
    def hex(self) -> str:
        return binascii.hexlify(self.data, sep=" ").decode("ascii")

    @property
    def textify(self) -> str:
        return "".join(chr(x) for x in self.data)

    def __str__(self) -> str:
        return f"Message(length={self.length}, hex={self.hex}, text={self.textify})"

    def as_protocol_message(self) -> Optional[ProtocolMessage]:
        try:
            data = self.data

            for code, msg_type in MESSAGES.items():
                if data.startswith(code.encode("ascii")):
                    return msg_type.from_bytes(data)

            return None
        except (UnicodeDecodeError, IndexError, KeyError, struct.error) as e:
            logging.debug(f"Failed to parse message: {e}")
            return None


@dataclass
class Connection:
    """Represents a socket connection with its associated logger."""

    socket: socket.socket
    logger: logging.Logger
    peer_addr: Tuple[str, int]
    buffer: bytearray = field(
        default_factory=bytearray
    )  # Buffer for incomplete messages


@dataclass
class HostPort:
    """Represents a host and port combination."""

    host: str
    port: int

    @classmethod
    def from_string(cls, addr: str) -> Self:
        if ":" in addr:
            host, port = addr.rsplit(":", 1)
            return cls(host, int(port))
        return cls(addr, DEFAULT_PORT)


def next_message(buffer: bytearray) -> Optional[Message]:
    """Process the buffer and return a complete message if available."""
    if len(buffer) < 4:
        return None

    length = int.from_bytes(buffer[:4], byteorder="big")
    total_size = length + 4  # Include the length prefix

    if len(buffer) < total_size:
        return None

    msg = Message(length, buffer[4:total_size])
    del buffer[:total_size]
    return msg


def handle_connection(source: Connection, dest: Connection, filters: list[str]) -> None:
    """Handle data transfer between source and destination connections."""
    try:
        data = source.socket.recv(BUFFER_SIZE)
        if not data:
            raise socket.error("Connection closed")

        # Add received data to the buffer
        source.buffer.extend(data)
        source.logger.debug(
            f"Received {len(data)} bytes: {binascii.hexlify(data, sep=' ')} {''.join(chr(x) for x in data)}"
        )

        while msg := next_message(source.buffer):
            source.logger.debug(msg)

            # Try to identify and parse the protocol message
            protocol_message = msg.as_protocol_message()
            if protocol_message:
                if not filters or protocol_message.code not in filters:
                    source.logger.info(protocol_message)
            else:
                source.logger.warning(f"Unknown message: {msg}")

            # Forward the complete message including its length prefix
            dest.socket.sendall(len(msg.data).to_bytes(4, byteorder="big") + msg.data)

    except socket.error as e:
        if not isinstance(e, BlockingIOError):
            source.logger.error(f"Socket error: {e}")
        raise


def main():
    parser = argparse.ArgumentParser(
        description="Deskflow protocol debugger",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Use all defaults (listen on 24801, connect to localhost:24800)
  %(prog)s

  # Connect to a specific host (listen on 24801)
  %(prog)s otherhost

  # Connect to a specific host:port (listen on 24801)
  %(prog)s otherhost:12345

  # Listen on a different port
  %(prog)s --port 12345 otherhost

The debugger acts as a MITM proxy, logging all protocol messages that pass through it.
It listens for incoming connections and forwards them to the specified remote host.""",
    )
    parser.add_argument(
        "remote",
        nargs="?",
        default="localhost",
        help="Deskflow server host:port to connect to (defaults to localhost:24800)",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=24801,
        help="Port to listen on for incoming deskflow client connections (default: 24801)",
    )
    parser.add_argument(
        "--verbose",
        "-v",
        action="store_true",
        help="Enable verbose logging",
    )
    parser.add_argument(
        "--show-keycodes",
        action="store_true",
        default=False,
        help="Show keycodes in the logging output (by default all keys are shown as A)",
    )
    parser.add_argument(
        "--filter",
        type=str,
        default="CALV,CNOP",
        help="A comma-separated list of protocol codes to ignore (default:  CALV,CNOP)",
    )
    args = parser.parse_args()

    global show_keycodes
    show_keycodes = args.show_keycodes

    # Configure logging based on verbosity
    log_level = logging.DEBUG if args.verbose else logging.INFO
    logger.setLevel(log_level)
    clogger.setLevel(log_level)
    slogger.setLevel(log_level)


    filters = [
        ff for ff in (f.strip() for f in args.filter.split(",")) if ff
    ]
    logger.info(f"Filtering message codes: {filters}")

    # Parse remote address
    remote = HostPort.from_string(args.remote)

    # Create server socket
    server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_sock.bind(("", args.port))
    server_sock.listen(1)

    logger.info(
        f"Listening on port {args.port}, connecting to {remote.host}:{remote.port}"
    )

    while True:
        try:
            # Accept client connection
            client_sock, client_addr = server_sock.accept()
            logger.info(f"Client connected from {client_addr[0]}:{client_addr[1]}")

            # Connect to remote server
            remote_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            remote_sock.connect((remote.host, remote.port))
            logger.info(f"Connected to remote {remote.host}:{remote.port}")

            # Create connection objects
            client_conn = Connection(client_sock, clogger, client_addr)
            remote_conn = Connection(remote_sock, slogger, (remote.host, remote.port))

            # Set up polling
            poller = select.poll()
            poller.register(client_sock, select.POLLIN)
            poller.register(remote_sock, select.POLLIN)

            # Main event loop
            while True:
                try:
                    # Wait for events with a 1-second timeout
                    events = poller.poll(1000)  # timeout in milliseconds
                    for fd, event in events:
                        if event & select.POLLIN:
                            if fd == client_sock.fileno():
                                handle_connection(client_conn, remote_conn, filters)
                            elif fd == remote_sock.fileno():
                                handle_connection(remote_conn, client_conn, filters)
                        if event & (select.POLLHUP | select.POLLERR):
                            raise socket.error("Connection closed")

                except BlockingIOError:
                    continue
                except socket.error as e:
                    logger.error(f"Socket error: {e}")
                    break

        except KeyboardInterrupt:
            logger.info("Shutting down on request")
            break
        except Exception as e:
            logger.warning(f"Error: {e}")
            continue
        finally:
            try:
                poller.unregister(client_sock)
                poller.unregister(remote_sock)
                client_sock.close()
                remote_sock.close()
            except Exception:
                pass

    server_sock.close()


if __name__ == "__main__":
    main()
