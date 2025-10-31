/*
 * ESP32 MDP Configuration
 * =======================
 *
 * WiFi and network configuration for ESP32 MDP firmware.
 * Edit these values for your network setup.
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// WIFI CONFIGURATION
// ============================================================================

// 🔧 CHANGE THESE VALUES FOR YOUR NETWORK
const char* WIFI_SSID = "YourWiFiNetwork";
const char* WIFI_PASSWORD = "YourWiFiPassword";

// ============================================================================
// NETWORK CONFIGURATION
// ============================================================================

// 🔧 CHANGE THESE VALUES FOR YOUR NETWORK
// ESP32 Static IP Address
#define STATIC_IP_BYTE_1  192
#define STATIC_IP_BYTE_2  168
#define STATIC_IP_BYTE_3  1
#define STATIC_IP_BYTE_4  100

// Network Gateway
#define GATEWAY_BYTE_1    192
#define GATEWAY_BYTE_2    168
#define GATEWAY_BYTE_3    1
#define GATEWAY_BYTE_4    1

// Network Subnet Mask
#define SUBNET_BYTE_1     255
#define SUBNET_BYTE_2     255
#define SUBNET_BYTE_3     255
#define SUBNET_BYTE_4     0

// UDP Port for MDP communication
#define UDP_PORT          4040

// ============================================================================
// DEBUG CONFIGURATION
// ============================================================================

#define SERIAL_BAUD       115200
#define DEBUG_OUTPUT      true

#endif // CONFIG_H
