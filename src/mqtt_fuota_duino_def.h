/**
 * @file    mqtt_fuota_duino_def.h
 * @author  Jose Miguel Rios Rubio <jrios.github@gmail.com>
 * @date    12-05-2023
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

/* Constants */

// Maximum Application Size expected (set to 4MB Flash)
#if !defined(MAX_APP_SIZE)
    static const uint32_t MAX_APP_SIZE = 4194304U;
#endif

// MQTT Client Received messages Buffer Size
static const uint16_t RX_BUFFER_SIZE = 2048U;

// Buffer size to store each Firmware data block
static constexpr uint16_t RX_FW_DATA_BUFFER_SIZE = (RX_BUFFER_SIZE / 2U);

// Maximum Topic string length (i.e. "XX:XX:XX:XX:XX:XX/ota/control")
static const uint8_t MAX_TOPIC_LENGTH = 32U;

// Maximum Length of UUID
static const uint8_t MAX_UUID_LENGTH = MAX_TOPIC_LENGTH - 12U;

// Firmware Version Length (XXX.YYY.ZZZ - Major.Minor.Patch)
static const uint8_t FW_VERSION_LENGTH = 3U;

// Commands Frame Length
static const unsigned int CMD_LEN = 4U;

/*****************************************************************************/

/* MQTT Topics */

// Topic from Server to Setup Device:
// Trigger update check, Provide last FW version, FW update start message
static const char MQTT_TOPIC_SUB_OTA_SETUP[] = "/%s/ota/setup";

// Topic from Server to send Firmware Data messages
static const char MQTT_TOPIC_SUB_OTA_DATA[] = "/%s/ota/data";

// Topic from Device for OTA Control Requests:
// FW Update check (request last FW version), Request FW update,
// Notify Update completed
static const char MQTT_TOPIC_PUB_OTA_CONTROL[] = "/%s/ota/control";

// Topic from Device to Acknowledge of received data messages
static const char MQTT_TOPIC_PUB_OTA_ACK[] = "/%s/ota/ack";

/*****************************************************************************/

/* Include Guard Close */

#endif /* MQTT_FUOTA_DUINO_DEF_H */
