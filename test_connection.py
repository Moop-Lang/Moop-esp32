#!/usr/bin/env python3

"""
ESP32 WiFi Connection Test
==========================

Simple Python script to test if ESP32 is reachable over WiFi
before flashing the MDP firmware.

Usage:
    python3 test_connection.py [esp32_ip]

Example:
    python3 test_connection.py 192.168.1.100
"""

import socket
import sys
import time

def test_udp_connection(esp32_ip, port=4040, timeout=5):
    """Test UDP connection to ESP32"""
    try:
        # Create UDP socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(timeout)

        # Test message
        test_message = b"HELLO_ESP32"
        server_address = (esp32_ip, port)

        print(f"📡 Testing connection to {esp32_ip}:{port}...")

        # Send test message
        sock.sendto(test_message, server_address)
        print(f"✅ Sent: {test_message.decode()}")

        # Wait for response
        try:
            data, address = sock.recvfrom(1024)
            print(f"✅ Received: {data.decode()} from {address}")
            return True
        except socket.timeout:
            print("⏰ No response (timeout)")
            return False

    except Exception as e:
        print(f"❌ Error: {e}")
        return False
    finally:
        sock.close()

def test_tcp_connection(esp32_ip, port=80, timeout=5):
    """Test if ESP32 has a web server (common default)"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(timeout)

        print(f"🌐 Testing HTTP connection to {esp32_ip}:{port}...")

        result = sock.connect_ex((esp32_ip, port))
        if result == 0:
            print("✅ HTTP server found")
            return True
        else:
            print("❌ No HTTP server")
            return False

    except Exception as e:
        print(f"❌ Error: {e}")
        return False
    finally:
        sock.close()

def ping_esp32(esp32_ip):
    """Simple ping test"""
    import subprocess
    try:
        print(f"📶 Pinging {esp32_ip}...")
        result = subprocess.run(['ping', '-c', '3', '-W', '2', esp32_ip],
                              capture_output=True, text=True, timeout=10)

        if result.returncode == 0:
            print("✅ ESP32 is reachable")
            return True
        else:
            print("❌ ESP32 not reachable")
            print(f"Ping output: {result.stdout}")
            return False
    except Exception as e:
        print(f"❌ Ping error: {e}")
        return False

def main():
    # Default ESP32 IP
    esp32_ip = "192.168.1.100"

    # Check command line argument
    if len(sys.argv) > 1:
        esp32_ip = sys.argv[1]

    print("🧪 ESP32 WiFi Connection Test")
    print("=" * 40)
    print(f"Target IP: {esp32_ip}")
    print()

    # Test 1: Ping
    print("Test 1: Network Reachability")
    ping_ok = ping_esp32(esp32_ip)
    print()

    if not ping_ok:
        print("❌ ESP32 not reachable on network")
        print("💡 Check:")
        print("   1. ESP32 WiFi credentials")
        print("   2. Static IP configuration")
        print("   3. Same WiFi network")
        print("   4. ESP32 power and WiFi connection")
        return

    # Test 2: UDP (MDP port)
    print("Test 2: UDP Connection (MDP Port)")
    udp_ok = test_udp_connection(esp32_ip, 4040)
    print()

    # Test 3: HTTP (common ESP32 default)
    print("Test 3: HTTP Connection (Port 80)")
    http_ok = test_tcp_connection(esp32_ip, 80)
    print()

    # Summary
    print("📊 Test Summary")
    print("-" * 20)
    print(f"Network: {'✅' if ping_ok else '❌'}")
    print(f"UDP (MDP): {'✅' if udp_ok else '❌'}")
    print(f"HTTP: {'✅' if http_ok else '❌'}")
    print()

    if udp_ok:
        print("🎉 ESP32 MDP firmware is responding!")
        print("Ready to run: make demo")
    elif ping_ok:
        print("📡 ESP32 is online but MDP firmware not responding")
        print("Next: Flash the ESP32 firmware")
        print("Run: ./build_and_flash.sh")
    else:
        print("❌ ESP32 not reachable")
        print("Check WiFi configuration")

if __name__ == "__main__":
    main()
