/**
 * @file    mqtt_fuota_duino.h
 * @author  Jose Miguel Rios Rubio <jrios.github@gmail.com>
 * @date    14-05-2023
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
#include "mqtt_fuota_duino_def.h"

// MQTT Library
#include <PubSubClient.h>

/*****************************************************************************/

/* Class Interface */

/**
 * @brief MQTT Firmware Update Over The Air Class Interface.
 */
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
         * @param current_fw_info Current Device Application Firmware
         * information to set current FW version in MQTTFirmwareUpdate.
         * @param device_id Device identification string to be used on topics.
         * @return true Initialization success.
         * @return false Initialization fail.
         */
        bool init(
            PubSubClient* mqtt_client,
            t_fw_info current_fw_info,
            char* device_id=nullptr);

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

        /**
         * @brief Time to wait between topic subscription attempts in case of
         * error.
         */
        static const unsigned long T_SUBSCRIBE = 5000U;

        /**
         * @brief MQTT Client Received messages Buffer Size.
         */
        static constexpr uint16_t RX_BUFFER_SIZE = 1100U;

        /**
         * @brief Maximum Topic string length
         * (i.e. "xx:xx:xx:xx:xx:xx/ota/control").
         */
        static const uint8_t MAX_TOPIC_LENGTH = 32U;

        /**
         * @brief Maximum Length of UUID.
         */
        static const uint8_t MAX_UUID_LENGTH = MAX_TOPIC_LENGTH - 12U;

        #if !defined(MAX_APP_SIZE)
            /**
             * @brief Maximum Application Size expected (set to 4MB Flash).
             */
            static const uint32_t MAX_APP_SIZE = 4194304U;
        #endif

    /******************************************************************/

    /* Private Attributes */

    private:

        /**
         * @brief Status of component initialized.
         */
        bool is_initialized;

        /**
         * @brief Pointer to MQTT Client component to be used.
         */
        PubSubClient* MQTTClient;

        /**
         * @brief Initial time count for MQTT subscription.
         */
        unsigned long t0_subscribe;

        /**
         * @brief Flag to identify if the MQTT CLient is subscribed to the
         * Setup topic.
         */
        bool subcribed_to_topic_ota_setup;

        /**
         * @brief Flag to identify if the MQTT CLient is subscribed to the
         * Data topic.
         */
        bool subcribed_to_topic_ota_data;

        /**
         * @brief String of MQTT Subscription topic: Setup.
         */
        char topic_sub_ota_setup[MAX_TOPIC_LENGTH];

        /**
         * @brief String of MQTT Subscription topic: Data.
         */
        char topic_sub_ota_data[MAX_TOPIC_LENGTH];

        /**
         * @brief String of MQTT Publish topic: Control.
         */
        char topic_pub_ota_control[MAX_TOPIC_LENGTH];

        /**
         * @brief String of MQTT Publish topic: Acknowledge.
         */
        char topic_pub_ota_ack[MAX_TOPIC_LENGTH];

        /**
         * @brief Received FUOTA request from Server to be handled.
         */
        t_server_request server_request;

        /**
         * @brief Flag to be set if the Device accept to update the FW (the FW
         * information received is valid and an update can proceed).
         */
        bool valid_update;

        /**
         * @brief Flag to identify that a FUOTA process is in progress.
         */
        bool fuota_on_progress;

        /**
         * @brief Flag to identify that a FUOTA process has been completed.
         */
        bool fw_update_completed;

        /**
         * @brief Flag to identify that a new FW data block has been received.
         */
        bool fw_data_block_received;

        /**
         * @brief Counter of Firmware bytes written to memory during a FUOTA
         * process.
         */
        uint32_t fw_bytes_written;

        /**
         * @brief Last Firmware data block received during the FUOTA process.
         */
        uint32_t fw_block_n;

        /**
         * @brief Current Device Firmware information (size, version and
         * checksum).
         */
        t_fw_info fw_device;

        /**
         * @brief Server available Firmware information (size, version and
         * checksum).
         */
        t_fw_info fw_server;

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

        /**
         * @brief Handle any MQTT message received from the Server through
         * Setup topic.
         * @param payload Message pyload data received.
         * @param length Number of bytes of message pyload data received.
         */
        void mqtt_msg_rx_ota_setup(uint8_t* payload, uint32_t length);

        /**
         * @brief Handle any MQTT message received from the Server through
         * Data topic.
         * @param payload Message pyload data received.
         * @param length Number of bytes of message pyload data received.
         */
        void mqtt_msg_rx_ota_data(uint8_t* payload, uint32_t length);

    /******************************************************************/

    /* Private Methods - Auxiliary */

    private:

        /**
         * @brief Generate and get a default Device ID to be used on MQTT
         * topics. The default device ID generated is the device WiFi MAC
         * address (i.e. xx:xx:xx:xx:xx:xx/ota/data).
         * @param uuid Address of char array that is going to get and store
         * the generated device ID.
         * @param uuid_size Size of array that is going to store the ID.
         */
        void get_device_uuid(char* uuid, const uint32_t uuid_size);

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
         * @brief Publish "Control Command" MQTT message to the Control topic.
         * @param command Command data to publish.
         * @return true MQTT message publish success.
         * @return false MQTT message publish fail.
         */
        bool publish_control_command(const uint8_t* command);

        /**
         * @brief Publish "FW block ACK" MQTT message to the ACK topic.
         * @param block_num Number of Firmware block to acknowledge.
         * @return true MQTT message publish success.
         * @return false MQTT message publish fail.
         */
        bool publish_data_block_ack(const uint32_t block_num);

        /**
         * @brief Clear t_fw_info struct attributes to their default values.
         * @param fw_info Pointer to t_fw_info element to clear.
         */
        void t_fw_info_clear(t_fw_info* fw_info);

        /**
         * @brief Add independent version major-minor-patch uint8_t bytes into
         * a single uint32_t data type.
         * @param ver_x Version Major field (XXX.YYY.ZZZ).
         * @param ver_y Version Minor field (XXX.YYY.ZZZ).
         * @param ver_z Version Patch field (XXX.YYY.ZZZ).
         * @return uint32_t Version number word on 4 bytes.
         */
        uint32_t u32_version_from_array(
            const uint8_t ver_x,
            const uint8_t ver_y,
            const uint8_t ver_z);

        /**
         * @brief Add an uint32_t value into a byte array in Big Endian order.
         * @param u32_value uint32_t value to add into the the array.
         * @param array Pointer of array to store the bytes.
         */
        void big_endian_u32_write_to_array(
            const uint32_t u32_value,
            uint8_t* array);

        /**
         * @brief Get an uint32_t value from byte array reading it in Big
         * Endian order.
         * @param array Pointer of array to read the bytes from.
         * @return uint32_t Value read.
         */
        uint32_t big_endian_u32_read_from_array(uint8_t* array);

};

/*****************************************************************************/

/* Include Guard Close */

#endif /* MQTT_FUOTA_DUINO_H */
