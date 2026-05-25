#include <Arduino.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <WebSocketsClient.h>
#include <WiFi.h>

namespace {

constexpr char WIFI_SSID[] = "mimi";
constexpr char WIFI_PASSWORD[] = "123456780";

// default 69,169.97.215:8000/ws/device/sensor-data/ for prduction
constexpr char WS_HOST[] = "69.169.97.215";
constexpr uint16_t WS_PORT = 8000;
constexpr char WS_PATH[] = "/ws/device/sensor-data/";
constexpr char DEVICE_ID[] = "esp32_01";

constexpr uint8_t RELAY_PIN = 26;
constexpr uint8_t ONE_WIRE_BUS_PIN = 4;
constexpr bool RELAY_ACTIVE_HIGH = true;

constexpr unsigned long WIFI_RETRY_MS = 5000;
constexpr unsigned long HEARTBEAT_INTERVAL_MS = 5000;
constexpr unsigned long SENSOR_INTERVAL_MS = 10000;

WebSocketsClient webSocket;
OneWire oneWire(ONE_WIRE_BUS_PIN);

bool pumpOn = false;
bool tempSensorAvailable = false;
unsigned long lastWifiRetryAt = 0;
unsigned long lastHeartbeatAt = 0;
unsigned long lastSensorAt = 0;

void setPumpState(bool on) {
  pumpOn = on;
  digitalWrite(RELAY_PIN, (RELAY_ACTIVE_HIGH ? on : !on) ? HIGH : LOW);
  Serial.printf("Pump state set to %s\n", on ? "ON" : "OFF");
}

void sendJson(const JsonDocument& doc) {
  String payload;
  serializeJson(doc, payload);
  webSocket.sendTXT(payload);
  Serial.println(payload);
}

void sendHeartbeat() {
  JsonDocument doc;
  doc["type"] = "heartbeat";
  doc["device_id"] = DEVICE_ID;
  sendJson(doc);
}

void sendPumpState() {
  JsonDocument doc;
  doc["type"] = "pump_state";
  doc["command"] = pumpOn ? "ON" : "OFF";
  doc["device_id"] = DEVICE_ID;
  sendJson(doc);
}

void sendSensorReading(float temperatureC) {
  JsonDocument doc;
  doc["type"] = "sensor_reading";
  doc["temperature"] = temperatureC;
  doc["device_id"] = DEVICE_ID;
  sendJson(doc);
}

void handleSocketMessage(const char* payload, size_t length) {
  JsonDocument doc;
  const auto err = deserializeJson(doc, payload, length);
  if (err) {
    Serial.printf("Failed to parse socket message: %s\n", err.c_str());
    return;
  }

  const char* type = doc["type"] | "";
  if (strcmp(type, "pump_command") == 0) {
    const char* command = doc["command"] | "";
    const bool turnOn = strcmp(command, "ON") == 0;
    const bool turnOff = strcmp(command, "OFF") == 0;
    if (turnOn || turnOff) {
      setPumpState(turnOn);
      sendPumpState();
    }
    return;
  }

  if (strcmp(type, "error") == 0) {
    Serial.printf("Server error: %s\n", static_cast<const char*>(doc["message"] | ""));
  }
}

void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("WebSocket disconnected");
      break;
    case WStype_CONNECTED:
      Serial.println("WebSocket connected");
      sendHeartbeat();
      sendPumpState();
      break;
    case WStype_TEXT:
      handleSocketMessage(reinterpret_cast<const char*>(payload), length);
      break;
    case WStype_ERROR:
      Serial.println("WebSocket error");
      break;
    default:
      break;
  }
}

void connectWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  const unsigned long now = millis();
  if (now - lastWifiRetryAt < WIFI_RETRY_MS) {
    return;
  }

  lastWifiRetryAt = now;
  Serial.printf("Connecting to Wi-Fi SSID %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void setupWebSocket() {
  webSocket.begin(WS_HOST, WS_PORT, WS_PATH);
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
}

}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(RELAY_PIN, OUTPUT);
  setPumpState(false);

  WiFi.mode(WIFI_STA);
  connectWifi();

  setupWebSocket();
}

void loop() {
  connectWifi();

  if (WiFi.status() == WL_CONNECTED) {
    webSocket.loop();

    const unsigned long now = millis();
    if (now - lastHeartbeatAt >= HEARTBEAT_INTERVAL_MS) {
      lastHeartbeatAt = now;
      sendHeartbeat();
    }

    if (pumpOn && now - lastSensorAt >= SENSOR_INTERVAL_MS) {
      lastSensorAt = now;
      const float temperatureC = readTemperatureC();
      if (isnan(temperatureC)) {
        Serial.println("Temperature read unavailable");
      } else {
        sendSensorReading(temperatureC);
      }
    }
  }
}
