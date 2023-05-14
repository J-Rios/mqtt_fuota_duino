/**
 * @file    mqtt_fuota_basic.ino
 * @author  Jose Miguel Rios Rubio <jrios.github@gmail.com>
 * @date    14-05-2023
 * @version 1.0.0
 *
 * @section DESCRIPTION
 *
 * Basic example of mqtt_fuota_duino library usage.
 *
 * This example show how to use the mqtt_fuota_duino library to get the FUOTA
 * over MQTT functionality available in the device. As requirement for this,
 * an established WiFi connection and a MQTT client setup is required, so this
 * example cover the initialization and connection of WiFi and MQTT in order
 * to use the MQTTFirmwareUpdate element of the mqtt_fuota_duino library.
 *
 * @section LICENSE
 *
 * MIT License
 *
 * Copyright (c) 2023 Jose Miguel Rios Rubio
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*****************************************************************************/

/* Libraries */

// Standard C/C++ libraries

// Device/Framework libraries (Arduino ESP32/ESP8266 Cores)
#include <Arduino.h>
#ifdef ESP8266
    #include <ESP8266WiFi.h>
#else
    #include <WiFi.h>
#endif

// Custom libraries
#include <mqtt_fuota_duino.h>

/*****************************************************************************/

/* Constants & Configurations */

// WiFi Access Point
#if !defined(SET_WIFI_SSID)
    #define SET_WIFI_SSID "mynet1234"
#endif
#if !defined(SET_WIFI_PASS)
    #define SET_WIFI_PASS "password1234"
#endif
static const char WIFI_SSID[] = SET_WIFI_SSID;
static const char WIFI_PASS[] = SET_WIFI_PASS;
static const uint8_t MAX_LENGTH_WIFI_SSID = 31;
static const uint8_t MAX_LENGTH_WIFI_PASS = 63;
static const uint8_t MAX_CONN_FAIL = 50U;

// Device Hostname to set for MQTT
static const char DEVICE_HOST_NAME[] = "mqtt_fuota_device";

// MQTT Server Connection
static const char MQTT_SERVER[] = "test.mosquitto.org";
static const uint16_t MQTT_PORT = 1883;

// Current Firmware Application Version (XXX.YYY.ZZZ)
static const uint8_t FW_APP_VERSION_X = 1U;
static const uint8_t FW_APP_VERSION_Y = 0U;
static const uint8_t FW_APP_VERSION_Z = 0U;

/*****************************************************************************/

/* Functions Prototypes */

bool mqtt_connect();
bool mqtt_is_connected();
static void cb_mqtt_msg_rx(char* topic, uint8_t* payload, unsigned int length);
bool wifi_handle_connection();
void wifi_init_stat();

/*****************************************************************************/

/* Global Elements */

WiFiClient WifiCl;
PubSubClient MQTTClient(WifiCl);
MQTTFirmwareUpdate MqttFuota;

/*****************************************************************************/

/* Main Function */

void setup()
{
    // Initialize Serial
    Serial.begin(115200);
    Serial.println("System Start");
    Serial.print("FW App version: ");
    Serial.printf("v%d.%d.%d\n",
                  FW_APP_VERSION_X, FW_APP_VERSION_Y, FW_APP_VERSION_Z);

    // Initialize WiFi station connection
    wifi_init_stat();

    // Setup MQTT Client
    MQTTClient.setServer(MQTT_SERVER, MQTT_PORT);
    MQTTClient.setCallback(cb_mqtt_msg_rx);

    // MQTT FUOTA Initialization
    t_fw_info app_info;
    app_info.version[0] = FW_APP_VERSION_X;
    app_info.version[1] = FW_APP_VERSION_Y;
    app_info.version[2] = FW_APP_VERSION_Z;
    if (MqttFuota.init(&MQTTClient, app_info) == false)
    {   Serial.println("Error - MQTT FUOTA Initialization Fail");   }
}

void loop()
{
    // Handle WiFi connection
    bool wifi_connected = wifi_handle_connection();

    // Do nothing if WiFi is not connected yet (wait 100ms before retry)
    if (wifi_connected == false)
    {   delay(100); return;   }

    // Handle MQTT
    if (mqtt_is_connected() == false)
    {
        if (mqtt_connect())
        {   Serial.println("App ready for receive FW Update");   }
    }
    else
    {
        // Process MQTT client
        MQTTClient.loop();

        // Process MQTT FUOTA
        MqttFuota.process();
    }
}

/*****************************************************************************/

/* Functions */

void wifi_init_stat()
{
    Serial.println("Initializing TCP-IP adapter...");
    Serial.print("Wifi connecting to SSID: ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    Serial.println("TCP-IP adapter successfuly initialized.");
}

/*****************************************************************************/

/* WiFi Change Event Handler */

bool wifi_handle_connection()
{
    static bool wifi_connected = false;

    // Device connected
    if (WiFi.status() == WL_CONNECTED)
    {
        // Show debug message if wasn't connected
        if (wifi_connected == false)
        {
            Serial.println("");
            Serial.println("WiFi connected");
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());
        }
        wifi_connected = true;
    }

    // Device not connected
    else
    {
        // Show debug message if was connected
        if(wifi_connected)
        {   Serial.println("WiFi disconnected.");   }
        wifi_connected = false;
    }

    return wifi_connected;
}

/*****************************************************************************/

/* MQTT Messages Reception Function Callback */

static void cb_mqtt_msg_rx(char* topic, uint8_t* payload, unsigned int length)
{
    //Serial.println("MQTT msg rx");

    // MQTT FUOTA Mechanism
    MqttFuota.mqtt_msg_rx(topic, payload, (uint32_t)(length));

    // Check here your application topics
    // ...
}

/*****************************************************************************/

/* MQTT Functions */

bool mqtt_is_connected()
{
    return (bool)(MQTTClient.connected());
}

bool mqtt_connect()
{
    static const unsigned long T_WAIT_RECONNECT = 5000U;
    static unsigned long t0 = 0U;

    // Do nothing if time for connect/reconnect has not arrive
    if (millis() - t0 < T_WAIT_RECONNECT)
    {   return false;   }
    t0 = millis();

    // MQTT Connection
    Serial.println("MQTT Connection...");
    if (MQTTClient.connect(WiFi.macAddress().c_str()) == false)
    {
        Serial.println("Fail to connect to MQTT Broker...\n");
        return false;
    }

    // Connection Success
    t0 = 0U;
    Serial.println("MQTT Connected\n");

    // Subscribe here to any needed topic
    // ...

    return mqtt_is_connected();
}
