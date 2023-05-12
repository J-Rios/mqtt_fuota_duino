/**
 * @file    mqtt_fuota_duino.h
 * @author  Jose Miguel Rios Rubio <jrios.github@gmail.com>
 * @date    12-05-2023
 * @version 1.0.0
 *
 * @section DESCRIPTION
 *
 * Header file of mqtt_fuota_duino library interface.
 *
 * This file contains the header of all the functions related to the library,
 * like the class interface.
 * It also specify the declaration of the extern class object that is defined
 * on the source file (this is done to expose an already instantiated object
 * to the Arduino user).
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

/* Include Guard */

#ifndef MQTT_FUOTA_DUINO_H
#define MQTT_FUOTA_DUINO_H

/*****************************************************************************/

/* Libraries */

// Standard C++ Libraries
#include <cstdint>

// MQTT FUOTA Protocol Definitions
#include "mqtt_fuota_def.h"

// MQTT Library
#include <PubSubClient.h>

/*****************************************************************************/

/* Class Interface */

class MQTTFirmwareUpdate
{
    /* Constructor & Destructor */

    public:

        /**
         * @brief Construct a new MQTTFirmwareUpdate object.
         */
        MQTTFirmwareUpdate();

        /**
         * @brief Destroy the MQTTFirmwareUpdate object.
         */
        ~MQTTFirmwareUpdate();

    /******************************************************************/

    /* Public Methods */

    public:

        /**
         * @brief Initialize the MQTTFirmwareUpdate component.
         * @param mqtt_client MQTT Client to use.
         * @param device_id Device identification string to be used on topics.
         * @return true Initialization success.
         * @return false Initialization fail.
         */
        bool init(PubSubClient* mqtt_client, char* device_id=nullptr);

        /**
         * @brief Run an iteration of the MQTTFirmwareUpdate main behaviour.
         */
        void process();

        /**
         * @brief Provide a received MQTT message to the MQTTFirmwareUpdate
         * component to let it check and handle it.
         * @param topic Received MQTT message topic.
         * @param payload Received MQTT message payload data.
         * @param length Received MQTT message payload data number of bytes.
         * @return true The message was expected and handled by the
         * MQTTFirmwareUpdate component.
         * @return false The message was not expected and was ignored by the
         * MQTTFirmwareUpdate component.
         */
        bool mqtt_msg_rx(char* topic, uint8_t* payload, uint32_t length);

    /******************************************************************/

    /* Private Constants */

    private:


    /******************************************************************/

    /* Private Data Types */

    private:


    /******************************************************************/

    /* Private Attributes */

    private:

        bool is_initialized;
        PubSubClient* MQTTClient;
        bool fw_ready_update;
        unsigned long t0_subscribe;
        bool subcribed_to_topic_ota_setup;
        bool subcribed_to_topic_ota_data;
        char topic_sub_ota_setup[MAX_TOPIC_LENGTH];
        char topic_sub_ota_data[MAX_TOPIC_LENGTH];
        char topic_pub_ota_control[MAX_TOPIC_LENGTH];
        char topic_pub_ota_ack[MAX_TOPIC_LENGTH];

    /******************************************************************/

    /* Private Methods - FUOTA */

    private:

        /**
         * @brief Handle requests received through the Setup topic.
         */
        void handle_server_requests();

        /**
         * @brief Handle received Firmware data blocks and send acknowledges.
         */
        void handle_received_fw_data();

    /******************************************************************/

    /* Private Methods - Auxiliary */

    private:

        /**
         * @brief Check if MQTT is connected.
         * @return true MQTT is connected.
         * @return false MQTT is disconnected.
         */
        bool is_connected();

        /**
         * @brief Check for FUOTA MQTT subscriptions and subscribe if needed.
         */
        void manage_subscriptions();

        /**
         * @brief Generate and get a default Device ID to be used on MQTT
         * topics. The default device ID generated is the device WiFi MAC
         * address (i.e. xx:xx:xx:xx:xx:xx/ota/data).
         * @param uuid Address of char array that is going to get and store
         * the generated device ID.
         * @param uuid_size Size of array that is going to store the ID.
         */
        void get_device_uuid(char* uuid, const uint32_t uuid_size);

};

/*****************************************************************************/

/* Object Declaration */

extern MQTTFirmwareUpdate MQTTFUOTA;

/*****************************************************************************/

/* Include Guard Close */

#endif /* MQTT_FUOTA_DUINO_H */
