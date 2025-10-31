/*
 * ESP32 MDP L3 Implementation
 * ==========================
 *
 * ESP32 firmware for testing MDP (Moop Datagram Protocol) communication
 * with M1 Mac over WiFi. Implements L3 actor system with UDP server.
 *
 * Features:
 * - WiFi connectivity
 * - UDP server for MDP packets
 * - Basic actor spawn/kill operations
 * - Heartbeat responses
 * - Serial debugging
 */

// ============================================================================
// CONFIGURATION
// ============================================================================

// WiFi Configuration
const char* WIFI_SSID = "YourWiFiNetwork";        // 🔧 CHANGE THIS
const char* WIFI_PASSWORD = "YourWiFiPassword";   // 🔧 CHANGE THIS

// Network Configuration
IPAddress STATIC_IP(192, 168, 1, 100);           // 🔧 CHANGE THIS
IPAddress GATEWAY(192, 168, 1, 1);
IPAddress SUBNET(255, 255, 255, 0);
const int UDP_PORT = 4040;

// Serial Configuration
#define SERIAL_BAUD 115200
#define DEBUG true

// ============================================================================
// INCLUDES
// ============================================================================

#include <WiFi.h>
#include <WiFiUdp.h>

// ============================================================================
// MDP PROTOCOL DEFINITIONS
// ============================================================================

// MDP Message Types
#define MDP_TYPE_SPAWN      0x01
#define MDP_TYPE_KILL       0x02
#define MDP_TYPE_SEND       0x03
#define MDP_TYPE_STATE      0x04
#define MDP_TYPE_BIND       0x05
#define MDP_TYPE_HEARTBEAT  0x06

// MDP Status Codes
#define MDP_STATUS_SUCCESS             0x00
#define MDP_STATUS_ERROR_INVALID_ACTOR 0x01
#define MDP_STATUS_ERROR_PORT_BUSY     0x02
#define MDP_STATUS_ERROR_MEMORY_FULL   0x03

// MDP Constants
#define MDP_VERSION       0x01
#define MDP_HEADER_SIZE   16
#define MDP_MAX_PAYLOAD   1184
#define UDP_BUFFER_SIZE   2048

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

WiFiUDP udp;
uint8_t udpBuffer[UDP_BUFFER_SIZE];
uint32_t nextActorId = 1000;
uint32_t heartbeatCount = 0;

// Actor registry (simple array for demo)
#define MAX_ACTORS 10
struct Actor {
    uint32_t id;
    bool active;
    uint32_t messageCount;
};

Actor actors[MAX_ACTORS];

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void debugPrint(const char* message) {
    if (DEBUG) {
        Serial.println(message);
    }
}

void debugPrintf(const char* format, ...) {
    if (DEBUG) {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        Serial.print(buffer);
    }
}

// CRC16 calculation (same as M1 implementation)
uint16_t crc16(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

// ============================================================================
// ACTOR MANAGEMENT
// ============================================================================

int32_t spawnActor() {
    for (int i = 0; i < MAX_ACTORS; i++) {
        if (!actors[i].active) {
            actors[i].id = nextActorId++;
            actors[i].active = true;
            actors[i].messageCount = 0;

            debugPrintf("Spawned actor ID: %u\n", actors[i].id);
            return actors[i].id;
        }
    }

    debugPrint("ERROR: No free actor slots");
    return -1; // No free slots
}

bool killActor(uint32_t actorId) {
    for (int i = 0; i < MAX_ACTORS; i++) {
        if (actors[i].active && actors[i].id == actorId) {
            actors[i].active = false;
            debugPrintf("Killed actor ID: %u\n", actorId);
            return true;
        }
    }

    debugPrintf("ERROR: Actor ID %u not found\n", actorId);
    return false;
}

bool isValidActor(uint32_t actorId) {
    for (int i = 0; i < MAX_ACTORS; i++) {
        if (actors[i].active && actors[i].id == actorId) {
            return true;
        }
    }
    return false;
}

uint32_t getActorMessageCount(uint32_t actorId) {
    for (int i = 0; i < MAX_ACTORS; i++) {
        if (actors[i].active && actors[i].id == actorId) {
            return actors[i].messageCount;
        }
    }
    return 0;
}

void incrementActorMessageCount(uint32_t actorId) {
    for (int i = 0; i < MAX_ACTORS; i++) {
        if (actors[i].active && actors[i].id == actorId) {
            actors[i].messageCount++;
            break;
        }
    }
}

// ============================================================================
// MDP MESSAGE HANDLING
// ============================================================================

void sendMdpResponse(uint32_t reqId, uint8_t status, const uint8_t* payload = NULL, uint16_t payloadLen = 0) {
    // Build MDP response packet
    uint8_t responseBuffer[UDP_BUFFER_SIZE];
    memset(responseBuffer, 0, sizeof(responseBuffer));

    // Header (big-endian)
    responseBuffer[0] = MDP_VERSION;           // Version
    responseBuffer[1] = 0x00;                  // Response type (0 for responses)
    responseBuffer[2] = 0x00;                  // Flags
    responseBuffer[3] = 0x00;                  // Reserved
    *(uint32_t*)&responseBuffer[4] = reqId;    // Request ID
    *(uint32_t*)&responseBuffer[8] = 0;        // Actor ID (0 for system responses)
    *(uint16_t*)&responseBuffer[12] = payloadLen; // Payload length
    responseBuffer[15] = status;               // Status in reserved field

    // Copy payload
    if (payload && payloadLen > 0 && payloadLen <= MDP_MAX_PAYLOAD) {
        memcpy(&responseBuffer[16], payload, payloadLen);
    }

    // Calculate CRC16
    uint16_t crc = crc16(responseBuffer, MDP_HEADER_SIZE + payloadLen);
    *(uint16_t*)&responseBuffer[14] = crc;

    // Send via UDP
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.write(responseBuffer, MDP_HEADER_SIZE + payloadLen);
    udp.endPacket();

    debugPrintf("Sent MDP response: status=%d, payloadLen=%d\n", status, payloadLen);
}

void handleSpawnRequest(uint32_t reqId) {
    int32_t actorId = spawnActor();

    if (actorId >= 0) {
        // Success response with actor ID
        sendMdpResponse(reqId, MDP_STATUS_SUCCESS, (uint8_t*)&actorId, sizeof(actorId));
    } else {
        // Error response
        sendMdpResponse(reqId, MDP_STATUS_ERROR_MEMORY_FULL);
    }
}

void handleKillRequest(uint32_t reqId, uint32_t actorId) {
    if (killActor(actorId)) {
        sendMdpResponse(reqId, MDP_STATUS_SUCCESS);
    } else {
        sendMdpResponse(reqId, MDP_STATUS_ERROR_INVALID_ACTOR);
    }
}

void handleSendMessage(uint32_t reqId, const uint8_t* payload, uint16_t payloadLen) {
    if (payloadLen < 8) {  // Minimum size for target_actor_id + message_size
        sendMdpResponse(reqId, MDP_STATUS_ERROR_INVALID_ACTOR);
        return;
    }

    uint32_t targetActorId = *(uint32_t*)&payload[0];
    uint16_t messageSize = *(uint16_t*)&payload[4];

    if (isValidActor(targetActorId)) {
        incrementActorMessageCount(targetActorId);
        debugPrintf("Message delivered to actor %u (size: %d)\n", targetActorId, messageSize);
        sendMdpResponse(reqId, MDP_STATUS_SUCCESS);
    } else {
        debugPrintf("Invalid target actor: %u\n", targetActorId);
        sendMdpResponse(reqId, MDP_STATUS_ERROR_INVALID_ACTOR);
    }
}

void handleStateRequest(uint32_t reqId, uint32_t actorId) {
    if (!isValidActor(actorId)) {
        sendMdpResponse(reqId, MDP_STATUS_ERROR_INVALID_ACTOR);
        return;
    }

    // Build state payload: [flags, message_count]
    uint8_t statePayload[8];
    statePayload[0] = 1;  // ACTIVE flag
    statePayload[1] = 0;  // Priority (not implemented)
    *(uint16_t*)&statePayload[2] = getActorMessageCount(actorId); // Message count
    *(uint16_t*)&statePayload[4] = 0; // Reserved

    sendMdpResponse(reqId, MDP_STATUS_SUCCESS, statePayload, sizeof(statePayload));
}

void handleBindRequest(uint32_t reqId) {
    // Simple bind response - always succeed for demo
    uint32_t bindingId = 2000;
    sendMdpResponse(reqId, MDP_STATUS_SUCCESS, (uint8_t*)&bindingId, sizeof(bindingId));
}

void handleHeartbeat(uint32_t reqId, const uint8_t* payload, uint16_t payloadLen) {
    heartbeatCount++;

    debugPrintf("Heartbeat #%u received", heartbeatCount);

    if (payloadLen >= 6) {  // timestamp + sequence
        uint32_t timestamp = *(uint32_t*)&payload[0];
        uint16_t sequence = *(uint16_t*)&payload[4];
        debugPrintf(" (seq: %u, ts: %u)", sequence, timestamp);
    }
    debugPrint("");

    // Echo heartbeat back
    sendMdpResponse(reqId, MDP_STATUS_SUCCESS, payload, payloadLen);
}

void processMdpPacket(const uint8_t* packet, size_t packetLen) {
    if (packetLen < MDP_HEADER_SIZE) {
        debugPrint("ERROR: Packet too small");
        return;
    }

    // Parse header (big-endian)
    uint8_t version = packet[0];
    uint8_t type = packet[1];
    uint8_t flags = packet[2];
    // uint8_t reserved = packet[3];
    uint32_t reqId = *(uint32_t*)&packet[4];
    uint32_t actorId = *(uint32_t*)&packet[8];
    uint16_t payloadLen = *(uint16_t*)&packet[12];
    uint16_t crc = *(uint16_t*)&packet[14];

    // Validate version
    if (version != MDP_VERSION) {
        debugPrintf("ERROR: Invalid MDP version: %d\n", version);
        return;
    }

    // Validate payload length
    if (payloadLen > MDP_MAX_PAYLOAD) {
        debugPrintf("ERROR: Payload too large: %d\n", payloadLen);
        return;
    }

    // Validate total packet size
    size_t expectedSize = MDP_HEADER_SIZE + payloadLen;
    if (packetLen != expectedSize) {
        debugPrintf("ERROR: Size mismatch. Got %zu, expected %zu\n", packetLen, expectedSize);
        return;
    }

    // Validate CRC
    uint16_t calculatedCrc = crc16(packet, expectedSize);
    if (calculatedCrc != crc) {
        debugPrintf("ERROR: CRC mismatch. Got 0x%04X, expected 0x%04X\n", crc, calculatedCrc);
        return;
    }

    debugPrintf("Processing MDP packet: type=0x%02X, reqId=%u, actorId=%u, payloadLen=%d\n",
                type, reqId, actorId, payloadLen);

    // Dispatch based on message type
    const uint8_t* payload = &packet[16];

    switch (type) {
        case MDP_TYPE_SPAWN:
            handleSpawnRequest(reqId);
            break;

        case MDP_TYPE_KILL:
            handleKillRequest(reqId, actorId);
            break;

        case MDP_TYPE_SEND:
            handleSendMessage(reqId, payload, payloadLen);
            break;

        case MDP_TYPE_STATE:
            handleStateRequest(reqId, actorId);
            break;

        case MDP_TYPE_BIND:
            handleBindRequest(reqId);
            break;

        case MDP_TYPE_HEARTBEAT:
            handleHeartbeat(reqId, payload, payloadLen);
            break;

        default:
            debugPrintf("ERROR: Unknown message type: 0x%02X\n", type);
            sendMdpResponse(reqId, MDP_STATUS_ERROR_INVALID_ACTOR);
            break;
    }
}

// ============================================================================
// SETUP AND LOOP
// ============================================================================

void setup() {
    // Initialize serial
    Serial.begin(SERIAL_BAUD);
    debugPrint("\n=== ESP32 MDP L3 Starting ===");

    // Initialize actor registry
    memset(actors, 0, sizeof(actors));
    debugPrint("Actor registry initialized");

    // Connect to WiFi
    debugPrint("Connecting to WiFi...");
    WiFi.config(STATIC_IP, GATEWAY, SUBNET);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        debugPrintf("\nWiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        debugPrint("\nWiFi connection failed!");
        return;
    }

    // Start UDP server
    udp.begin(UDP_PORT);
    debugPrintf("UDP server started on port %d\n", UDP_PORT);

    debugPrint("=== ESP32 MDP L3 Ready ===");
    debugPrint("Send MDP packets from M1 Mac to test communication");
}

void loop() {
    // Check for UDP packets
    int packetSize = udp.parsePacket();
    if (packetSize > 0) {
        // Read packet
        int len = udp.read(udpBuffer, sizeof(udpBuffer));
        if (len > 0) {
            debugPrintf("Received UDP packet: %d bytes from %s:%d\n",
                       len, udp.remoteIP().toString().c_str(), udp.remotePort());

            // Process MDP packet
            processMdpPacket(udpBuffer, len);
        }
    }

    // Small delay to prevent busy loop
    delay(10);
}
