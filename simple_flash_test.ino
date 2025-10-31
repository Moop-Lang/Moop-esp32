/*
 * ESP32 Simple WiFi Test
 * ======================
 *
 * Basic test to verify ESP32 can connect to WiFi.
 * Upload this first, then check your router for the ESP32.
 */

#include <WiFi.h>

// 🔧 CHANGE THESE FOR YOUR NETWORK
const char* WIFI_SSID = "YourWiFiNetwork";
const char* WIFI_PASSWORD = "YourWiFiPassword";

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n=== ESP32 WiFi Test Starting ===");

    // Connect to WiFi
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("✅ WiFi connected!");
        Serial.print("📡 ESP32 IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("📶 Signal strength: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");

        // Blink LED to show success
        pinMode(LED_BUILTIN, OUTPUT);
        for(int i = 0; i < 10; i++) {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(100);
            digitalWrite(LED_BUILTIN, LOW);
            delay(100);
        }
    } else {
        Serial.println("❌ WiFi connection failed!");
        Serial.println("Check your WiFi credentials in the sketch");
    }

    Serial.println("=== Setup Complete ===");
}

void loop() {
    // Keep WiFi connected
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected, reconnecting...");
        WiFi.reconnect();
        delay(5000);
    }

    // Print status every 30 seconds
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 30000) {
        Serial.print("📡 IP: ");
        Serial.print(WiFi.localIP());
        Serial.print(" | 📶 RSSI: ");
        Serial.println(WiFi.RSSI());
        lastPrint = millis();
    }

    delay(1000);
}
