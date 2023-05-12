/**
 * @file    mqtt_fuota_duino.cpp
 * @author  Jose Miguel Rios Rubio <jrios.github@gmail.com>
 * @date    12-05-2023
 * @version 1.0.0
 *
 * @section DESCRIPTION
 *
 * Source file of mqtt_fuota_duino library implementation.
 *
 * This file contains the implementation of all the functions related to the
 * library, like the class methods.
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

/* Firmware Update Procedure */

/**
 * Server                                                       Device
 *   ||                                                           ||
 *   || Trigger FW Update Check [device_id/ota/setup]             ||
 *   ||---------------------------------------------------------->||
 *   ||                                                           ||
 *   ||                   FW Update Check [device_id/ota/control] ||
 *   ||<----------------------------------------------------------||
 *   ||                                                           ||
 *   || Last Stable FW Version Info [device_id/ota/setup]         ||
 *   ||---------------------------------------------------------->||
 *   ||                                                           ||
 *   ||                 Request FW Update [device_id/ota/control] ||
 *   ||<----------------------------------------------------------||
 *   ||                                                           ||
 *   || FUOTA Start [device_id/ota/setup]                         ||
 *   ||---------------------------------------------------------->||
 *   ||                                                           ||
 *   ||                       ACK FUOTA Start [device_id/ota/ack] ||
 *   ||<----------------------------------------------------------||
 *   ||                                                           ||
 *   || FW Data Block 0 [device_id/ota/data]                      ||
 *   ||---------------------------------------------------------->||
 *   ||                                                           ||
 *   ||                   ACK FW Data Block 0 [device_id/ota/ack] ||
 *   ||<----------------------------------------------------------||
 *   ||                                                           ||
 *   ||                          .                                ||
 *   ||                          .                                ||
 *   ||                          .                                ||
 *   ||                                                           ||
 *   || FW Data Block N [device_id/ota/data]                      ||
 *   ||---------------------------------------------------------->||
 *   ||                                                           ||
 *   ||                   ACK FW Data Block N [device_id/ota/ack] ||
 *   ||<----------------------------------------------------------||
 *   ||                                                           ||
 *   ||               FW Update Completed [device_id/ota/control] ||
 *   ||<----------------------------------------------------------||
 *   ||                                                           ||
 */

/*****************************************************************************/

/* Libraries */

// Header Interface
#include "mqtt_fuota_duino.h"

// Hardware Abstraction Layer Framework
#include "Arduino.h"

// Firmware Update Library
#include "Update.h"

// WiFi Library
#include <WiFi.h>

/*****************************************************************************/

/* Object Instantiation */

MQTTFirmwareUpdate MQTTFUOTA;

/*****************************************************************************/

/* Constructor & Destructor */

MQTTFirmwareUpdate::MQTTFirmwareUpdate()
{
    // Initialize Class Attributes
    is_initialized = false;
    MQTTClient = nullptr;
    t0_subscribe = 0U - T_SUBSCRIBE;
    subcribed_to_topic_ota_setup = false;
    subcribed_to_topic_ota_data = false;
    topic_pub_ota_control[0] = '\0';
    topic_sub_ota_data[0] = '\0';
    topic_sub_ota_setup[0] = '\0';
    topic_pub_ota_ack[0] = '\0';
}

MQTTFirmwareUpdate::~MQTTFirmwareUpdate()
{   /* Nothing to do */   }

/*****************************************************************************/

/* Public Methods */

bool MQTTFirmwareUpdate::init(PubSubClient* mqtt_client, char* device_id)
{
    char _device_id[MAX_UUID_LENGTH];

    // Check if provided WiFi Client is valid
    if (mqtt_client == nullptr)
    {   return false;   }

    // Do nothing if component is already initialized
    if (is_initialized)
    {   return true;   }

    MQTTClient = mqtt_client;

    // Setup Device ID if not provided
    if (device_id == nullptr)
    {
        device_id = _device_id;
        get_device_uuid(device_id, MAX_UUID_LENGTH);
    }

    // Setup topics
    snprintf(topic_sub_ota_setup, sizeof(topic_sub_ota_setup),
        MQTT_TOPIC_SUB_OTA_SETUP, device_id);
    snprintf(topic_sub_ota_data, sizeof(topic_sub_ota_data),
        MQTT_TOPIC_SUB_OTA_DATA, device_id);
    snprintf(topic_pub_ota_control, sizeof(topic_pub_ota_control),
        MQTT_TOPIC_PUB_OTA_CONTROL, device_id);
    snprintf(topic_pub_ota_ack, sizeof(topic_pub_ota_ack),
        MQTT_TOPIC_PUB_OTA_ACK, device_id);

    // Set MQTT Client required Rx buffer size if needed
    if (MQTTClient->getBufferSize() < RX_BUFFER_SIZE)
    {   MQTTClient->setBufferSize(RX_BUFFER_SIZE);   }

    is_initialized = true;
    return true;
}

void MQTTFirmwareUpdate::process()
{
    // Do nothing if component is not initialized
    if (is_initialized == false)
    {   return;   }

    // Do nothing if MQTT is not connected
    if (is_connected() == false)
    {   return;   }

    // Check for subscriptions and resubscribe if needed
    manage_subscriptions();

    // Process MQTT client
    MQTTClient->loop();

    // Handle all requests sent by the Server through the Setup topic
    handle_server_requests();

    // Handle Firmware data block receptions and acknowledges
    handle_received_fw_data();
}

bool MQTTFirmwareUpdate::mqtt_msg_rx(char* topic, uint8_t* payload,
        uint32_t length)
{
    bool msg_handled = false;

    // Do nothing if not subscribed
    if ( (subcribed_to_topic_ota_setup == false)
      || (subcribed_to_topic_ota_data  == false) )
    {   return false;   }

    // Check for expected topics
    if (strcmp(topic, topic_sub_ota_setup) == 0)
    {
        Serial.printf("[MQTT_FUOTA] MSG RX: OTA Setup\n");
        mqtt_msg_rx_ota_setup(payload, length);
        msg_handled = true;
    }
    else if (strcmp(topic, topic_sub_ota_data) == 0)
    {
        Serial.printf("[MQTT_FUOTA] MSG RX: FW DATA\n");
        mqtt_msg_rx_ota_data(payload, length);
        msg_handled = true;
    }

    return msg_handled;
}

/*****************************************************************************/

/* Private Methods: FUOTA */

/**
 * @details This function handles all received requests from the Server
 * through the Setup topic.
 */
void MQTTFirmwareUpdate::handle_server_requests()
{
    // TODO
}

/**
 * @details This function handles all received firmware data blocks from the
 * Server through the data topic, validate and send the corresponding
 * acknowledges through the ack topic.
 */
void MQTTFirmwareUpdate::handle_received_fw_data()
{
    // TODO
}

/*****************************************************************************/

/* Private Methods - Auxiliary */

void MQTTFirmwareUpdate::get_device_uuid(char* uuid, const uint32_t uuid_size)
{
    snprintf(uuid, uuid_size, "%s", WiFi.macAddress().c_str());
}

bool MQTTFirmwareUpdate::is_connected()
{
    // Check if component is not initialized
    if (is_initialized == false)
    {   return false;   }

    return (bool)(MQTTClient->connected());
}

void MQTTFirmwareUpdate::manage_subscriptions()
{
    static const uint8_t sub_qos = 1U;

    // Do nothing if component is not initialized
    if (is_initialized == false)
    {   return;   }

    // Do nothing if already subscribed
    if (subcribed_to_topic_ota_setup && subcribed_to_topic_ota_data)
    {   return;   }

    // Do nothing if time for a new subscription attemp has not come yet
    if (millis() - t0_subscribe < T_SUBSCRIBE)
    {   return;   }
    t0_subscribe = millis();

    // Do nothing if MQTT is not connected
    if (is_connected() == false)
    {   return;   }

    // Subscribe to OTA Setup Topic
    if (subcribed_to_topic_ota_setup == false)
    {
        // Safe check that topic is valid
        if (topic_sub_ota_setup[0] != '\0')
        {
            subcribed_to_topic_ota_setup =
                MQTTClient->subscribe(topic_sub_ota_setup, sub_qos);
        }
    }

    // Subscribe to OTA Data Topic
    if (subcribed_to_topic_ota_data == false)
    {
        // Safe check that topic is valid
        if (topic_sub_ota_data[0] != '\0')
        {
            subcribed_to_topic_ota_data =
                MQTTClient->subscribe(topic_sub_ota_data, sub_qos);
        }
    }
}

/*****************************************************************************/
