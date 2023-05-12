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

/*****************************************************************************/

/* Object Instantiation */

MQTTFirmwareUpdate MQTTFUOTA;

/*****************************************************************************/

/* Constructor & Destructor */

MQTTFirmwareUpdate::MQTTFirmwareUpdate()
{
    // Initialize Class Attributes
}

MQTTFirmwareUpdate::~MQTTFirmwareUpdate()
{   /* Nothing to do */   }

/*****************************************************************************/

/* Public Methods */


/*****************************************************************************/

/* Private Methods */


/*****************************************************************************/
