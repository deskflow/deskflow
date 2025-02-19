"""
Common utilities for Deskflow security scanners
"""

import ssl
import socket
import time

def create_ssl_context():
    """Create a standard SSL context for Deskflow scanning"""
    context = ssl.create_default_context()
    context.check_hostname = False
    context.verify_mode = ssl.CERT_NONE
    context.minimum_version = ssl.TLSVersion.TLSv1_2
    context.maximum_version = ssl.TLSVersion.TLSv1_2
    context.set_ciphers('ALL:@SECLEVEL=0')
    return context

def frame_message(payload, is_initial=False):
    """Frame a message according to the Deskflow protocol format"""
    if is_initial:
        return b"\x16\x03\x03\x00\x14\x00\x00\x00\x0b\x53\x79\x6e\x65\x72\x67\x79\x00\x01\x00\x08"
    
    MAX_CHUNK_SIZE = 16384 - 32
    
    if len(payload) <= MAX_CHUNK_SIZE:
        msg = b"\x00\x00\x00\x0b\x53\x79\x6e\x65\x72\x67\x79\x00\x01\x00\x08" + payload
        msg_len = len(msg)
        tls_header = b"\x16\x03\x03" + msg_len.to_bytes(2, byteorder='big')
        return tls_header + msg
    else:
        chunks = []
        for i in range(0, len(payload), MAX_CHUNK_SIZE):
            chunk = payload[i:i + MAX_CHUNK_SIZE]
            msg = b"\x00\x00\x00\x0b\x53\x79\x6e\x65\x72\x67\x79\x00\x01\x00\x08" + chunk
            msg_len = len(msg)
            tls_header = b"\x16\x03\x03" + msg_len.to_bytes(2, byteorder='big')
            chunks.append(tls_header + msg)
        return b"".join(chunks)

def normalize_host(host):
    """Remove tls:// prefix if present"""
    if host.startswith('tls://'):
        return host[6:]
    return host

def send_with_retry(ssock, data, retries=3, timeout=5.0, delay=1.0):
    """Send data with retry logic and delay"""
    for attempt in range(retries):
        try:
            ssock.settimeout(timeout)
            ssock.sendall(data)
            time.sleep(delay)
            return True
        except (socket.error, ssl.SSLError) as e:
            if attempt == retries - 1:
                raise
            time.sleep(delay * (attempt + 1))
    return False

def receive_with_timeout(ssock, size=4096, timeout=5.0):
    """Receive data with timeout"""
    ssock.settimeout(timeout)
    try:
        return ssock.recv(size)
    except socket.timeout:
        return None

def create_ssl_socket(host, port, timeout=5.0):
    """Create and configure SSL socket with proper handshake"""
    context = create_ssl_context()
    sock = socket.create_connection((normalize_host(host), port), timeout=timeout)
    ssl_sock = context.wrap_socket(sock, server_hostname=host)
    ssl_sock.settimeout(timeout)
    return ssl_sock