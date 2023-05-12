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


    /******************************************************************/

    /* Private Constants */

    private:


    /******************************************************************/

    /* Private Data Types */

    private:


    /******************************************************************/

    /* Private Attributes */

    private:


    /******************************************************************/

    /* Private Methods */

    private:


};

/*****************************************************************************/

/* Object Declaration */

extern MQTTFirmwareUpdate MQTTFUOTA;

/*****************************************************************************/

/* Include Guard Close */

#endif /* MQTT_FUOTA_DUINO_H */
