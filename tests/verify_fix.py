import socket
import time
import os
import shutil
import subprocess
import sys

def send_command(sock, cmd):
    sock.sendall((cmd + "\n").encode())
    time.sleep(0.1)
    return sock.recv(1024).decode()

def test_user_isolation():
    # Cleanup
    if os.path.exists(".data/alice"):
        shutil.rmtree(".data/alice")
    if os.path.exists(".data/bob"):
        shutil.rmtree(".data/bob")

    # Start server
    server_process = subprocess.Popen(["./server", "8081"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    time.sleep(1) # Wait for server to start

    try:
        # Connect Alice
        sock_alice = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock_alice.connect(("localhost", 8081))
        sock_alice.sendall(b"alice") # Send username
        print(f"Alice connected: {sock_alice.recv(1024).decode().strip()}")

        # Connect Bob
        sock_bob = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock_bob.connect(("localhost", 8081))
        sock_bob.sendall(b"bob") # Send username
        print(f"Bob connected: {sock_bob.recv(1024).decode().strip()}")

        # Alice creates DB
        print("Alice creating db_alice...")
        send_command(sock_alice, "CREATE DATABASE db_alice")

        # Bob creates DB
        print("Bob creating db_bob...")
        send_command(sock_bob, "CREATE DATABASE db_bob")

        # Verify
        alice_db_path = ".data/alice/db_alice"
        bob_db_path = ".data/bob/db_bob"
        alice_has_bob_db = ".data/alice/db_bob"
        bob_has_alice_db = ".data/bob/db_alice"

        if os.path.exists(alice_db_path):
            print("PASS: .data/alice/db_alice exists")
        else:
            print("FAIL: .data/alice/db_alice does NOT exist")

        if os.path.exists(bob_db_path):
            print("PASS: .data/bob/db_bob exists")
        else:
            print("FAIL: .data/bob/db_bob does NOT exist")

        if not os.path.exists(alice_has_bob_db):
            print("PASS: .data/alice/db_bob does NOT exist")
        else:
            print("FAIL: .data/alice/db_bob EXISTS (Leak!)")

        if not os.path.exists(bob_has_alice_db):
            print("PASS: .data/bob/db_alice does NOT exist")
        else:
            print("FAIL: .data/bob/db_alice EXISTS (Leak!)")

    finally:
        sock_alice.close()
        sock_bob.close()
        server_process.terminate()
        server_process.wait()

if __name__ == "__main__":
    test_user_isolation()
