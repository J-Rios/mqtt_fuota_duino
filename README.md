# mqtt_fuota_duino

Let's update the firmware of your IoT devices through MQTT protocol!

This is a C++ library that provides the functionality of doing Firmware Update Over The Air (FUOTA), through MQTT protocol, for IoT devices based on Arduino software core, like Espressif ESP32 devices.

**Note:** This MQTT FUOTA component uses a MQTT PubSubClient library component as MQTT interface to use.

# Usage

Include the library in your code:

```c++
// MQTT FUOTA Library
#include "mqtt_fuota_duino.h"
```

You need to have some constants that identify your current firmware application version, for example:

```c++
// Current Firmware Application Version (XXX.YYY.ZZZ)
static const uint8_t FW_APP_VERSION_X = 1U;
static const uint8_t FW_APP_VERSION_Y = 0U;
static const uint8_t FW_APP_VERSION_Z = 0U;
```

Then you need to initialize to instantiate an object of MQTT FUOTA:

```c++
WiFiClient WifiCl;
PubSubClient MQTTClient(WifiCl);
MQTTFirmwareUpdate MqttFuota;
```

For the initialization of the MQTT FUOTA componment, you need to provide a PubSubClient MQTT object to it, and also give the current FW Application version:

```c++
// MQTT FUOTA Initialization
t_fw_info app_info;
app_info.version[0] = FW_APP_VERSION_X;
app_info.version[1] = FW_APP_VERSION_Y;
app_info.version[2] = FW_APP_VERSION_Z;
if (MqttFuota.init(&MQTTClient, app_info) == false)
{   Serial.println("Error - MQTT FUOTA Initialization Fail");   }
```

To make run the MQTT FUOTA component main behaviour, you need to use the process method in a main loop (i.e. on Arduino loop() function):

```c++
void loop()
{
    // ...

    // Process MQTT FUOTA
    MqttFuota.process();

    // ...
}
```

Last thing, on the MQTT message reception callback, you need to pass the received messages to the MQTT FUOTA component:

```c++
void setup()
{
    // ...

    // Set MQTT messages reception callback
    MQTTClient.setCallback(cb_mqtt_msg_rx);

    // ...
}

static void cb_mqtt_msg_rx(char* topic, uint8_t* payload, unsigned int length)
{
    // ...

    // Gives the message to the MQTT FUOTA Component
    MqttFuota.mqtt_msg_rx(topic, payload, (uint32_t)(length));

    // ...
}
```

# Server Side

The FUOTA Mechanism at Server side need to be developed according to the protocol definition. As a proof of concept, a python script tool is provided on the **tool** directory to force and ease a FW Update on a device.

**Note:** The device WiFi MAC address is required as target device ID to update.

Example of use:

```bash
cd tool
python3 mqtt_fuota_update.py --device 12:34:56:78:90:AB --firmware firmware.bin
```

# MQTT

The next MQTT topics are used by this FUOTA-MQTT mechanism:

- /{DEVICE_ID}/ota/setup - Used by Server to request force the device to trigger an update check, provide last available FW information on Server, launch a FW Update Start request.

- /{DEVICE_ID}/ota/control - Used by Device to request last available FW information on Server, request a FW Update, and notify Update completed (success or fail).

- /{DEVICE_ID}/ota/data - Used by Server to send FW data blocks to the Device.

- /{DEVICE_ID}/ota/ack - Used by a Device to acknowledge the reception of FW data blocks received.

# FUOTA Protocol

The MQTT messages transaction flow is the following:

```text
Server                                                       Device
  ||                                                           ||
  || Trigger FW Update Check [device_id/ota/setup]             ||
  ||---------------------------------------------------------->||
  ||                                                           ||
  ||                   FW Update Check [device_id/ota/control] ||
  ||<----------------------------------------------------------||
  ||                                                           ||
  || Last Stable FW Version Info [device_id/ota/setup]         ||
  ||---------------------------------------------------------->||
  ||                                                           ||
  ||                 Request FW Update [device_id/ota/control] ||
  ||<----------------------------------------------------------||
  ||                                                           ||
  || FUOTA Start [device_id/ota/setup]                         ||
  ||---------------------------------------------------------->||
  ||                                                           ||
  ||                       ACK FUOTA Start [device_id/ota/ack] ||
  ||<----------------------------------------------------------||
  ||                                                           ||
  || FW Data Block 0 [device_id/ota/data]                      ||
  ||---------------------------------------------------------->||
  ||                                                           ||
  ||                   ACK FW Data Block 0 [device_id/ota/ack] ||
  ||<----------------------------------------------------------||
  ||                                                           ||
  ||                          .                                ||
  ||                          .                                ||
  ||                          .                                ||
  ||                                                           ||
  || FW Data Block N [device_id/ota/data]                      ||
  ||---------------------------------------------------------->||
  ||                                                           ||
  ||                   ACK FW Data Block N [device_id/ota/ack] ||
  ||<----------------------------------------------------------||
  ||                                                           ||
  ||               FW Update Completed [device_id/ota/control] ||
  ||<----------------------------------------------------------||
  ||                                                           ||
```

# Notes

- **Compatibility:** This library is focus on ESP32 devices, but it is abstracted by Arduino framework, so other devices with WiFi support could be compatible.

- **Security:** The library relegates the security on the MQTT network itself, so is responsibility of the user to configure the MQTT client to use SSL/TLS for the communication between broker and client. Encryption of data on FUOTA protocol layer is not implemented.

- **Robustness**: This library uses different components and libraries of the Arduino Core, where some misuse of memory for embedded devices happens (i.e. dynamic memory usage and reallocation by PubSubClient MQTT library), so it is expected that the system could crash on run time at some point.
