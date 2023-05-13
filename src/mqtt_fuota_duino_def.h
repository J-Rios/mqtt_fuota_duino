/**
 * @file    mqtt_fuota_duino_def.h
 * @author  Jose Miguel Rios Rubio <jrios.github@gmail.com>
 * @date    13-05-2023
 * @version 1.0.0
 *
 * @section DESCRIPTION
 *
 * Definitions file of mqtt_fuota_duino library.
 *
 * This file contains different kind of definitions needed by the library
 * implementation or usage, like defines, constants, configurations,
 * data types, etc.
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

#ifndef MQTT_FUOTA_DUINO_DEF_H
#define MQTT_FUOTA_DUINO_DEF_H

/*****************************************************************************/

/* Libraries */

// Standard C++ Libraries
#include <cstdint>
#include <cstddef>

/*****************************************************************************/

/* MQTT Topics */

// Topic from Server to Setup Device:
// Trigger update check, Provide last FW version, FW update start message
static const char MQTT_TOPIC_SUB_OTA_SETUP[] = "/%s/ota/setup";

// Topic from Server to send Firmware Data messages
static const char MQTT_TOPIC_SUB_OTA_DATA[] = "/%s/ota/data";

// Topic from Device for OTA Control Requests:
// FW Update check (request last FW version information), Request FW update,
// Notify Update completed
static const char MQTT_TOPIC_PUB_OTA_CONTROL[] = "/%s/ota/control";

// Topic from Device to Acknowledge of received data messages
static const char MQTT_TOPIC_PUB_OTA_ACK[] = "/%s/ota/ack";

/*****************************************************************************/

/* Constants - Setup Message Commands (Server to Device) */

// Message to force the device to trigger a FW update check
static const uint8_t MSG_SETUP_CMD_TRIGGER_FW_UPDATE_CHECK = 0x00U;
static const uint8_t MSG_SETUP_CMD_TRIGGER_FW_UPDATE_CHECK_LENGTH = 1U;

// Message to provide last stable FW information (version, size, checksum)
static const uint8_t MSG_SETUP_CMD_LAST_FW_INFO = 0x01U;
static const uint8_t MSG_SETUP_CMD_LAST_FW_INFO_LENGTH = 24U;

// Message to request the device to start the FUOTA process (listening for FW
// data block messages
static const uint8_t MSG_SETUP_CMD_FUOTA_START = 0x02U;
static const uint8_t MSG_SETUP_CMD_FUOTA_START_LENGTH = 1U;

// Commands Frame Length
static const unsigned int CMD_LEN = 4U;

// Firmware Version Length (XXX.YYY.ZZZ - Major.Minor.Patch)
static const uint8_t FW_VERSION_LENGTH = 3U;

// MD5 Hash algorithm string value length
static const uint32_t MD5_LENGTH = 32U;

/*****************************************************************************/

/* Constants - Control Message Commands (Device to Server) */

// Device request a FW Update Check to get last FW information from server
static const uint8_t MSG_CONTROL_CMD_FW_UPDATE_CHECK[] =
{ 0xafU, 0x12U, 0x34U, 0x56U };

// Device request to launch a FUOTA process to Server
static const uint8_t MSG_CONTROL_CMD_REQUEST_FW_UPDATE[] =
{ 0x55U, 0x55U, 0xffU, 0xffU };

// Device ready to start FUOTA process and handle reception of FW data blocks
static const uint8_t MSG_ACK_FUOTA_START[] =
{ 0xaaU, 0xaaU, 0xaaU, 0xaaU };

// FUOTA process completed successfully
static const uint8_t MSG_CONTROL_CMD_FW_UPDATE_COMPLETED_OK[] =
{ 0x55U, 0xaaU, 0xffU, 0xffU };

// FUOTA process completed but update on device has fail
static const uint8_t MSG_CONTROL_CMD_FW_UPDATE_COMPLETED_FAIL[] =
{ 0x55U, 0xaaU, 0x00U, 0x00U };

/*****************************************************************************/

/* Data Types */

// Firmware information data (version, size and CRC)
typedef struct t_fw_info
{
    uint8_t version[FW_VERSION_LENGTH];
    uint32_t size;
#if 0 /* Unused CRC (used MD5 instead) */
    uint32_t crc;
#endif
    char md5[MD5_LENGTH];
} t_fw_info;

// Server Setup Messages Requests commands
enum class t_server_request : uint8_t
{
    NONE = 0U,
    TRIGGER_FW_UPDATE_CHECK = 1U,
    FW_UPDATE = 2U,
    FUOTA_START = 3U,
};

// Setup Message Fields buffer index locations
enum t_msg_fw_info_field
{
    FW_INFO_CMD = 0,
    FW_INFO_VER_MAJOR = 1,
    FW_INFO_VER_MINOR = 2,
    FW_INFO_VER_PATCH = 3,
    FW_INFO_SIZE = 4,
#if 0 /* Unused (used MD5 instead) */
    FW_INFO_CRC = 8,
#endif
    FW_INFO_MD5 = 8,
};

/*****************************************************************************/

/* Include Guard Close */

#endif /* MQTT_FUOTA_DUINO_DEF_H */
