#include "pin_config.h" // Includes pin definitions for CS, INT, RST
#include <SPI.h>
#include <RH_RF95.h>    // RadioHead library for LoRa (RFM9x)

// --- NEW INCLUDES FOR WIFI AND MQTT ---
#include <WiFi.h>
#include <PubSubClient.h>

// =======================================
// 📡 LoRa Configuration
// =======================================
RH_RF95 rf95(RFM95_CS, RFM95_INT);
#define RF95_FREQ 915.0

// =======================================
// 🌐 WiFi & MQTT Configuration (!!! CHANGE THESE !!!)
// =======================================
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* MQTT_BROKER = "YOUR_MQTT_BROKER_ADDRESS"; // e.g., "broker.hivemq.com"
const int MQTT_PORT = 1883;
const char* MQTT_CLIENT_ID = "ESP32-LoRa-Gateway-1";
const char* MQTT_TOPIC = "lora/data/sensor"; // The topic to publish to

// =======================================
// Client Objects
// =======================================
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// --- Function Prototypes ---
void initializeSerial();
void initializeLoRa();
void connectWifi();
void reconnectMqtt();
void publishPacket(const char* id, const char* command, int rssi, float snr);
void parseAndDisplayPacket(const char* packet);
void listenForPacket();

// ===================================
// 🛠️ Subsystem Initialization Functions
// ===================================

/**
 * @brief Initializes the Serial communication for monitoring.
 */
void initializeSerial() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10); 
    }
    Serial.println("\n--- ESP32 LoRa MQTT Gateway Setup ---");
}

/**
 * @brief Configures and initializes the RFM95 LoRa module.
 */
void initializeLoRa() {
    Serial.println("Initializing LoRa radio...");
    // 1. Configure Radio Pins & Reset pulse
    pinMode(RFM95_RST, OUTPUT);
    digitalWrite(RFM95_RST, HIGH);
    digitalWrite(RFM95_RST, LOW);
    delay(10);
    digitalWrite(RFM95_RST, HIGH);
    delay(10);
    
    // 2. Initialize the RFM95 LoRa Module
    if (!rf95.init()) {
        Serial.println("FATAL: LoRa radio init failed. Check wiring/frequency.");
        while (1) { delay(100); }
    }
    Serial.println("LoRa radio init OK!");

    // 3. Set LoRa Frequency
    if (!rf95.setFrequency(RF95_FREQ)) {
        Serial.printf("FATAL: setFrequency failed for %.1f MHz\n", RF95_FREQ);
        while (1) { delay(100); }
    }
    rf95.setTxPower(13, false); // Set Tx power (for acknowledgements if needed)
    Serial.printf("LoRa frequency set to: %.1f MHz. Listening...\n", RF95_FREQ);
}

/**
 * @brief Connects the ESP32 to the specified Wi-Fi network.
 */
void connectWifi() {
    Serial.printf("Connecting to WiFi: %s", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        attempts++;
        if (attempts > 20) {
            Serial.println("\nFATAL: Failed to connect to WiFi.");
            // In a real application, you might restart or retry.
            // For now, we will halt setup.
            while(1) delay(1000); 
        }
    }

    Serial.println("\nWiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}

/**
 * @brief Reconnects the MQTT client to the broker if disconnected.
 */
void reconnectMqtt() {
    // Loop until we're reconnected
    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (mqttClient.connect(MQTT_CLIENT_ID)) {
            Serial.println("connected!");
            // Subscribe to any relevant command topics here if needed.
            // mqttClient.subscribe("lora/command/gateway"); 
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" Retrying in 5 seconds...");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}


// ===================================
// 📦 Core Logic Functions
// ===================================

/**
 * @brief Publishes the parsed LoRa data and signal metrics to the MQTT broker.
 * @param id The sensor ID.
 * @param command The command/data string.
 * @param rssi The Received Signal Strength Indicator.
 * @param snr The Signal-to-Noise Ratio.
 */
void publishPacket(const char* id, const char* command, int rssi, float snr) {
    // 1. Ensure MQTT is connected
    if (!mqttClient.connected()) {
        reconnectMqtt();
    }
    
    // 2. Format data as a JSON string
    char payload[256];
    // Example JSON: {"id": "SensorA", "command": "TEMP:25.5", "rssi": -85, "snr": 7.5}
    int len = snprintf(payload, sizeof(payload), 
                       "{\"id\":\"%s\", \"data\":\"%s\", \"rssi\":%d, \"snr\":%.1f}",
                       id, command, rssi, snr);
    
    // Check for buffer overflow just in case
    if (len >= sizeof(payload)) {
        Serial.println("Warning: MQTT payload truncated.");
    }

    // 3. Publish
    Serial.printf("Publishing to MQTT topic %s: %s\n", MQTT_TOPIC, payload);
    if (mqttClient.publish(MQTT_TOPIC, payload)) {
        Serial.println("MQTT Publish successful.");
    } else {
        Serial.println("MQTT Publish FAILED!");
    }
}


/**
 * @brief Parses a received packet string in the format "ID,command" and displays/transmits its parts.
 * @param packet The null-terminated C string received from the LoRa radio.
 */
void parseAndDisplayPacket(const char* packet) {
    // Get signal metrics before processing the data
    int rssi = rf95.lastRssi();
    float snr = rf95.lastSNR();
    
    // Create a mutable copy for parsing
    char buffer[RH_RF95_MAX_MESSAGE_LEN + 1]; 
    strncpy(buffer, packet, RH_RF95_MAX_MESSAGE_LEN);
    buffer[RH_RF95_MAX_MESSAGE_LEN] = '\0';

    char* comma_pos = strchr(buffer, ',');

    Serial.println("\n-----------------------------");
    Serial.println("--- NEW LORA PACKET RECEIVED ---");
    Serial.printf("Raw Data:  %s\n", packet);

    if (comma_pos) {
        *comma_pos = '\0'; // Null-terminate the ID
        char* id_str = buffer;
        char* command_str = comma_pos + 1; // Command starts after the comma

        // Display results to console
        Serial.printf("ID:        %s\n", id_str);
        Serial.printf("Command:   %s\n", command_str);
        Serial.printf("RSSI:      %d dBm\n", rssi);
        Serial.printf("SNR:       %.1f dB\n", snr);
        
        // --- TRANSMIT VIA MQTT ---
        publishPacket(id_str, command_str, rssi, snr);

    } else {
        Serial.println("Error: Packet format invalid (missing comma ','). Packet NOT sent to MQTT.");
    }
    
    Serial.println("-----------------------------");
}


/**
 * @brief Checks for and processes any incoming LoRa packets.
 */
void listenForPacket() {
    if (rf95.available()) {
        uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(buf);

        if (rf95.recv(buf, &len)) {
            buf[len] = '\0';
            parseAndDisplayPacket((const char*)buf);
        } else {
            Serial.println("Error: LoRa Receive failed (checksum or CRC error).");
        }
    }
    // Let the MQTT client handle any subscription callbacks
    mqttClient.loop();
    // Wait briefly before checking again
    delay(50);
}

// ===================================
// ⚙️ Main Arduino Functions
// ===================================

void setup() {
    initializeSerial();
    
    // Set up Wi-Fi and MQTT before LoRa, as network setup might take time
    connectWifi();
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    reconnectMqtt();

    initializeLoRa();
}

void loop() {
    // Check Wi-Fi and MQTT connection status and attempt reconnect if necessary
    if (WiFi.status() != WL_CONNECTED) {
        connectWifi();
    }
    if (!mqttClient.connected()) {
        reconnectMqtt();
    }

    // Check for incoming LoRa packets and process them
    listenForPacket();
}