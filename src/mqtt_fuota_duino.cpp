/**
 * @file    mqtt_fuota_duino.cpp
 * @author  Jose Miguel Rios Rubio <jrios.github@gmail.com>
 * @date    13-05-2023
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

/* Constructor & Destructor */

/**
 * @details The constructor of the MQTTFirmwareUpdate class initializes all
 * its internal attributes to a default initial values.
 */
MQTTFirmwareUpdate::MQTTFirmwareUpdate()
{
    is_initialized = false;
    MQTTClient = nullptr;
    t0_subscribe = 0U - T_SUBSCRIBE;
    subcribed_to_topic_ota_setup = false;
    subcribed_to_topic_ota_data = false;
    topic_pub_ota_control[0] = '\0';
    topic_sub_ota_data[0] = '\0';
    topic_sub_ota_setup[0] = '\0';
    topic_pub_ota_ack[0] = '\0';
    server_request = t_server_request::NONE;
    valid_update = false;
    fuota_on_progress = false;
    fw_update_completed = false;
    fw_bytes_written = 0U;
    t_fw_info_clear(&fw_device);
    t_fw_info_clear(&fw_server);
}

/**
 * @details The destructor of the MQTTFirmwareUpdate class do nothing, due
 * this component has been implemented without using any kind of dynamic
 * memory allocation, there is no need to free anything.
 */
MQTTFirmwareUpdate::~MQTTFirmwareUpdate()
{}

/*****************************************************************************/

/* Public Methods */

/**
 * @details This function initializes the MQTTFirmwareUpdate component.
 * It gets and assign the provided MQTT Client interface to be used and the
 * Device ID string to be used as prefix of the MQTT OTA related topics (note:
 * if device_id is not provided, the ESP WiFi MAC address will be used). It
 * also prepare the MQTT topics strings with the device ID and set the MQTT
 * client reception buffer size according to minimum required size for
 * expected Firmware data block size used by the defined MQTT FUOTA protocol.
 */
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

/**
 * @details This function handle the main behaviour of the MQTTFirmwareUpdate
 * component.
 * It does nothing if the component has not not been initialized or there is
 * no MQTT connection. Otherwise it manages the required MQTT topics
 * subscriptions, the process of the MQTT client component, and handle any
 * received request from the Server and the received FW data in regards of the
 * FUOTA process.
 */
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

/**
 * @details This function receives an user provided MQTT message data to make
 * the MQTTFirmwareUpdate component checks if it comes from an OTA related
 * topic (setup or data), and dispatch the message to the corresponding and
 * specific function that must to handle it.
 * Note: In case the subscriptions has not been done, the function do nothing.
 */
bool MQTTFirmwareUpdate::mqtt_msg_rx(char* topic, uint8_t* payload,
        uint32_t length)
{
    bool msg_handled = false;

    // Do nothing if any of the subscriptions has not been done
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

/* Private Methods: MQTT Messages Received Handlers */

/**
 * @details This function handle any MQTT message received from the Server
 * through Setup topic.
 * It checks for the expected commands (first byte) and for the expected
 * data frame lengths. In case an expected command and length is received, in
 * general it sets the value of the request attribute to let them be handle
 * during the "process" function. For the "Last FW Version Information"
 * message reception, it also parse and gets all the available Server FW
 * information.
 * The different expected requests:
 * - Trigger FW Update Check
 * - Last Stable FW version info (version, size and checksum)
 * - FUOTA Start
 */
void MQTTFirmwareUpdate::mqtt_msg_rx_ota_setup(uint8_t* payload,
        uint32_t length)
{
    // Do nothing if none payload data received
    if (length == 0U)
    {   return;   }

    // Server is requesting for device to trigger a Firmware Update Check, and
    // make it update FW if needed
    if (payload[FW_INFO_CMD] == MSG_SETUP_CMD_TRIGGER_FW_UPDATE_CHECK)
    {
        // Check for expected message length
        if (length != MSG_SETUP_CMD_TRIGGER_FW_UPDATE_CHECK_LENGTH)
        {   return;   }

        Serial.printf("Server requesting device to check for FW update\n");
        server_request = t_server_request::TRIGGER_FW_UPDATE_CHECK;
    }

    // Server is providing information (version, size and checksum) of last
    // stable Firmware available to update
    else if (payload[FW_INFO_CMD] == MSG_SETUP_CMD_LAST_FW_INFO)
    {
        // Check for expected message length
        if (length != MSG_SETUP_CMD_LAST_FW_INFO_LENGTH)
        {   return;   }

        // Get FW version
        fw_server.version[0] = payload[FW_INFO_VER_MAJOR];
        fw_server.version[1] = payload[FW_INFO_VER_MINOR];
        fw_server.version[2] = payload[FW_INFO_VER_PATCH];

        // Get FW size
        fw_server.size =
            big_endian_u32_read_from_array(&(payload[FW_INFO_SIZE]));

        #if 0 /* Unused CRC (used MD5 instead) */
        fw_server.crc =
            big_endian_u32_read_from_array(&(payload[FW_INFO_CRC]));
        #endif

        // Get and convert MD5 bytes to string of chars
        char* ptr_fw_md5 = fw_server.md5;
        uint8_t md5_bytes_length = (uint8_t)(MD5_LENGTH / 2U);
        for (int i = 0; i < md5_bytes_length; i++)
        {
            ptr_fw_md5 += sprintf(ptr_fw_md5, "%02X", payload[i+FW_INFO_MD5]);
        }

        Serial.printf("\n");
        Serial.printf("\nServer FW info received:\n");
        Serial.printf("FW Version: %d.%d.%d\n", (int)(fw_server.version[0]),
            (int)(fw_server.version[1]), (int)(fw_server.version[2]));
        Serial.printf("FW Size: %" PRIu32 "KB\n",
            (uint32_t)(fw_server.size / 1024U));
        Serial.printf("FW MD5 Hash: %s\n", fw_server.md5);
        Serial.printf("\n");

        server_request = t_server_request::FW_UPDATE;
    }

    // Server is notifying the start of Firmware Update process
    else if (payload[FW_INFO_CMD] == MSG_SETUP_CMD_FUOTA_START)
    {
        // Check for expected message length
        if (length != MSG_SETUP_CMD_FUOTA_START_LENGTH)
        {   return;   }

        Serial.printf("Server requesting FUOTA Start\n");
        server_request = t_server_request::FUOTA_START;
    }

    // Unexpected messages
    else
    {   Serial.printf("Unexpected msg\n");   }
}

/**
 * @details This function handle any MQTT message received from the Server
 * through Data topic.
 * It does nothing if a FUOTA start request has not been received previously
 * or if last available FW information in the Server has not been received or
 * the firmware version to update is lower than current one (fuota_on_progress
 * and valid_update flags). If the previous requirements are valid, then the
 * function write the recieved FW data block into the memory, count the number
 * of bytes already received and flashed, show the update progress, and check
 * if the number of bytes written are the same as the Server FW size.
 */
void MQTTFirmwareUpdate::mqtt_msg_rx_ota_data(uint8_t* payload,
        uint32_t length)
{
    size_t num_bytes_written = 0U;
    uint8_t progress = 0U;

    // Do nothing if FW info received from server is not valid or the process
    // has not started
    if (valid_update == false)
    {   return;   }
    if (fuota_on_progress == false)
    {   return;   }

    // Limit bytes to write if there is coming more than expected
    if (fw_bytes_written + length > fw_server.size)
    {   length = fw_server.size - fw_bytes_written;   }

    // Write FW data block into memory
    num_bytes_written = Update.write(payload, length);
    fw_bytes_written = fw_bytes_written + num_bytes_written;

    // Show current update progress
    progress = (uint8_t)(100 * (fw_bytes_written / fw_server.size));
    Serial.printf("Updating %" PRIu8 "%% (%" PRIu32 "/%" PRIu32 "\n",
        progress, fw_bytes_written, fw_server.size);

    // Check if FW update has been completed
    if (fw_bytes_written >= fw_server.size)
    {   fw_update_completed = true;   }
}

/*****************************************************************************/

/* Private Methods: FUOTA */

/**
 * @details This function handles all received requests from the Server
 * through the Setup topic.
 */
void MQTTFirmwareUpdate::handle_server_requests()
{
    // Server Request the Device to trigger a FW Update Check
    if (server_request == t_server_request::TRIGGER_FW_UPDATE_CHECK)
    {
        server_request = t_server_request::NONE;

        // Clear Server firmware info
        memset((void*)(fw_server.version), 0, FW_VERSION_LENGTH);
        fw_server.size = 0U;
        fw_server.crc = 0xffffffffU;
        fw_server.md5[0] = '\0';

        Serial.printf("MSG Control Send: FW Update Check\n");
        publish_control_command(MSG_CONTROL_CMD_FW_UPDATE_CHECK);
    }

    // Trigger FW Update Check
    if (server_request == t_server_request::FW_UPDATE)
    {
        server_request = t_server_request::NONE;

        // Do nothing if received FW info from Server is invalid
        if ( (fw_server.size == 0U) || (fw_server.size > MAX_APP_SIZE) )
        {   return;   }

        Serial.printf("\n");
        Serial.printf("FW Version:\n");
        Serial.printf("Device: %d.%d.%d\n", (int)(fw_device.version[0]),
            (int)(fw_device.version[1]), (int)(fw_device.version[2]));
        Serial.printf("Server: %d.%d.%d\n", (int)(fw_server.version[0]),
            (int)(fw_server.version[1]), (int)(fw_server.version[2]));

        // Convert FW version to 32 bits unsigned integer for comparison
        uint32_t u32_fw_device_ver = u32_version_from_array(
            fw_device.version[0], fw_device.version[1], fw_device.version[2]);
        uint32_t u32_fw_server_ver = u32_version_from_array(
            fw_server.version[0], fw_server.version[1], fw_server.version[2]);

        // Do nothing if Server FW version is lower or equal than current one
        // If Server report a FW version 0.0.0, the FW must be accepted
        if (u32_fw_server_ver != 0U)
        {
            if (u32_fw_server_ver <= u32_fw_device_ver)
            {
                Serial.printf("FW Version Server <= Device\n");
                Serial.printf("No need to update FW\n");
                return;
            }
        }

        // Request FW update if device FW version is lower than Server one
        valid_update = true;
        Serial.printf("MSG Control Send: Request FW Update\n");
        publish_control_command(MSG_CONTROL_CMD_REQUEST_FW_UPDATE);
    }

    // Acknowledge FUOTA process start request to Server
    if (server_request == t_server_request::FUOTA_START)
    {
        server_request = t_server_request::NONE;

        // Make sure to stop any update in progress before launching a new one
        Update.abort();

        // Enable Updater Component
        if (Update.begin(fw_server.size) == false)
        {
            Serial.printf("Not enough APP space for update\n");
            return;
        }

        // Set Firmware target MD5 hash
        Update.setMD5(fw_server.md5);

        // Notify the Server that Device is ready to receive FW data
        fw_bytes_written = 0U;
        memset((void*)(fw_server.md5), (int)('\0'), MD5_LENGTH);
        fuota_on_progress = true;
        Serial.printf("MSG Control Send: FUOTA Start ACK\n");
        publish_control_command(MSG_ACK_FUOTA_START);
    }
}

/**
 * @details This function handles all received firmware data blocks from the
 * Server through the data topic, validate and send the corresponding
 * acknowledges through the ack topic.
 */
void MQTTFirmwareUpdate::handle_received_fw_data()
{
    // Do nothing if FW info received from server is not valid or the process
    // has not started
    if (valid_update == false)
    {   return;   }
    if (fuota_on_progress == false)
    {   return;   }

    // Handle FW Update errors
    if (Update.hasError())
    {
        Serial.println(Update.errorString());
        Update.abort();
        publish_control_command(MSG_CONTROL_CMD_FW_UPDATE_COMPLETED_FAIL);
        return;
    }

    // Handle FW Update completed
    if (fw_update_completed)
    {
        fw_update_completed = false;
        fuota_on_progress = false;

        Serial.println("Update completed\n");

        // Handle any pending update and check for update errors
        Update.remaining();
        if (Update.hasError())
        {
            Serial.println(Update.errorString());
            Update.abort();
            publish_control_command(MSG_CONTROL_CMD_FW_UPDATE_COMPLETED_FAIL);
            return;
        }

        // Check Update end
        if (Update.end() == false)
        {
            Serial.printf("Update fail\n");
            Update.abort();
            publish_control_command(MSG_CONTROL_CMD_FW_UPDATE_COMPLETED_FAIL);
            return;
        }

        // FW Update success, reboot system
        publish_control_command(MSG_CONTROL_CMD_FW_UPDATE_COMPLETED_OK);
        Serial.printf("FW Update success\n");
        Serial.printf("Rebooting FW\n");
        Serial.printf("\n-------------------------------\n\n");
        delay(3000);
        ESP.restart();
    }
}

/*****************************************************************************/

/* Private Methods - Auxiliary */

/**
 * @details This function add WiFi interface MAC Address to a given UUID
 * string (to be used as device ID for the MQTT topics).
 */
void MQTTFirmwareUpdate::get_device_uuid(char* uuid, const uint32_t uuid_size)
{
    snprintf(uuid, uuid_size, "%s", WiFi.macAddress().c_str());
}

/**
 * @details This function check if there is MQTT connection.
 */
bool MQTTFirmwareUpdate::is_connected()
{
    // Check if component is not initialized
    if (is_initialized == false)
    {   return false;   }

    return (bool)(MQTTClient->connected());
}

/**
 * @details This function check and handle the MQTT topics subscriptions.
 * It checks if the topics are already subscribed, if not, it subscribes.
 * In case the subscription fail, a new attempt for subscription will be done
 * after some T_SUBSCRIBE retry time.
 */
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

/**
 * @details This function check if the component is initialized and the MQTT
 * is connected, then publish the command data provided to the Control topic.
 */
bool MQTTFirmwareUpdate::publish_control_command(const uint8_t* command)
{
    // Do nothing if component is not initialized
    if (is_initialized == false)
    {   return false;   }

    // Do nothing if MQTT is not connected
    if (is_connected() == false)
    {   return false;   }

    return (
        (bool)(MQTTClient->publish(topic_pub_ota_control, command, CMD_LEN))
    );
}

/**
 * @details This function check if the component is initialized and the MQTT
 * is connected, then create a Firmware Block ACK message frame and publish it
 * to the Control topic.
 */
bool MQTTFirmwareUpdate::publish_data_block_ack(const uint32_t block_num)
{
    // Do nothing if component is not initialized
    if (is_initialized == false)
    {   return false;   }

    // Do nothing if MQTT is not connected
    if (is_connected() == false)
    {   return false;   }

    // Prepare payload frame (Add block number in Big Endian)
    uint8_t payload[CMD_LEN];
    big_endian_u32_write_to_array(block_num, payload);

    // Publish the message
    return (bool)(MQTTClient->publish(topic_pub_ota_ack, payload, CMD_LEN));
}

/**
 * @details This function converts a XXX.YYY.ZZZ version format from individual
 * bytes, into a single 32 bits unsigned integer element (0x00XXYYZZ).
 */
uint32_t MQTTFirmwareUpdate::u32_version_from_array(
        const uint8_t ver_x, const uint8_t ver_y, const uint8_t ver_z)
{
    return (uint32_t)((ver_x << 16U) + (ver_y << 8U) + ver_z);
}

/**
 * @details This function add the provided uint32_t value into an array in
 * Big Endian order.
 */
void MQTTFirmwareUpdate::big_endian_u32_write_to_array(
        const uint32_t u32_value, uint8_t* array)
{
	array[0] = (uint8_t)(u32_value >> 24U);
	array[1] = (uint8_t)(u32_value >> 16U);
	array[2] = (uint8_t)(u32_value >> 8U);
	array[3] = (uint8_t)u32_value;
}

/**
 * @details This function get and return an uint32_t value from 4 bytes of an
 * array in Big Endian order.
 */
uint32_t MQTTFirmwareUpdate::big_endian_u32_read_from_array(uint8_t* array)
{
	return (
        (uint32_t)(array[0] << 24U)
	  | (uint32_t)(array[1] << 16U)
	  | (uint32_t)(array[2] << 8U)
	  | (uint32_t)(array[3])
    );
}

/**
 * @details This function initialize a t_fw_info struct attributes to their
 * default clear values.
 */
void t_fw_info_clear(t_fw_info* fw_info)
{
    fw_info->size = 0U;
    memset((void*)(fw_info->version), 0, FW_VERSION_LENGTH);
    memset((void*)(fw_info->md5), (int)('\0'), MD5_LENGTH);
}

/*****************************************************************************/
