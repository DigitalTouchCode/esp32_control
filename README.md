# ESP32 Control

This project contains firmware for an ESP32-based pump control device built with PlatformIO and the Arduino framework. The device connects to Wi-Fi, opens a WebSocket connection to a backend server, listens for pump control commands, and reports device status.

## Overview

The firmware is designed to:

- connect the ESP32 to a Wi-Fi network
- connect to a remote WebSocket endpoint
- receive pump control commands from the server
- switch a relay connected to the pump
- send heartbeat messages
- send pump state updates
- send temperature readings while the pump is running

## Hardware Setup

The current code assumes the following hardware mapping:

- ESP32 development board
- relay connected to GPIO 26
- 1-Wire temperature sensor connected to GPIO 4

## Software Stack

- PlatformIO
- Arduino framework for ESP32
- ArduinoJson
- WebSockets client library
- OneWire
- DallasTemperature

The dependencies are declared in [platformio.ini](/Users/mrc/Desktop/esp32/platformio.ini).

## Project Structure

- [src/main.cpp](/Users/mrc/Desktop/esp32/src/main.cpp): main application logic
- [platformio.ini](/Users/mrc/Desktop/esp32/platformio.ini): board configuration and dependencies
- [include/](/Users/mrc/Desktop/esp32/include): header files
- [lib/](/Users/mrc/Desktop/esp32/lib): private libraries
- [test/](/Users/mrc/Desktop/esp32/test): test files

## Configuration

```cpp
constexpr char WIFI_SSID[] = "";
constexpr char WIFI_PASSWORD[] = "";
constexpr char WS_HOST[] = "";
constexpr uint16_t WS_PORT = 8000;
constexpr char WS_PATH[] = "/ws/device/sensor-data/";
constexpr char DEVICE_ID[] = "esp32_01";
```

Pin assignments are also defined in the same file:

```cpp
constexpr uint8_t RELAY_PIN = 26;
constexpr uint8_t ONE_WIRE_BUS_PIN = 4;
```

## Build And Upload

Run the following commands from the project directory:

```bash
pio run
pio run --target upload
pio device monitor -b 115200
```

## Runtime Behavior

When the firmware starts, it:

1. initializes serial communication
2. configures the relay output pin
3. connects to Wi-Fi
4. opens a WebSocket connection to the server
5. sends heartbeat and pump state messages
6. listens for incoming WebSocket commands

If a `pump_command` message is received, the relay state is updated and the new pump state is sent back to the server.

## Message Examples

Example heartbeat message:

```json
{"type":"heartbeat","device_id":"esp32_01"}
```

Example pump state message:

```json
{"type":"pump_state","command":"ON","device_id":"esp32_01"}
```

Example incoming command:

```json
{"type":"pump_command","command":"OFF"}
```

## Current Limitation

The source code calls `readTemperatureC()`, but that function is not implemented in the current project. Because of that, the firmware will not compile until temperature reading logic is added or the call is removed from [src/main.cpp](/Users/mrc/Desktop/esp32/src/main.cpp).

## Improvements To Make

- move credentials and server settings out of source code
- implement the temperature sensor reading function
- add better reconnect and error handling
- add automated tests for message parsing and device state changes
