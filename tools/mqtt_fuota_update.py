#!/usr/bin/env python3
# -*- coding: utf-8 -*-

'''
Script:
    mqtt_fuota_update.py
Description:
    Script to manage an Over The Air Firmware update of IoT devices
    through MQTT.
Author:
    Jose Miguel Rios Rubio
Date:
    14/05/2023
Version:
    1.0.0
Update Procedure:
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
'''

###############################################################################
# Tool Version Information
###############################################################################

NAME = __file__
VERSION = "1.0.0"
DATE = "14/05/2023"


###############################################################################
# Standard Libraries
###############################################################################

# Argument Parser Library
from argparse import ArgumentParser

# Logging Library
import logging

# Error Traceback Library
from traceback import format_exc

# Operating System Library
from os import path as os_path

# System Signals Library
from platform import system as os_system
from signal import signal, SIGTERM, SIGINT
if os_system() != "Windows":
    from signal import SIGUSR1

# System Library
from sys import argv as sys_argv
from sys import exit as sys_exit

# Cryptographic Library
import hashlib

# Time Library
import time

# Threads Library
import threading


###############################################################################
# Third-Party Libraries
###############################################################################

# MQTT Client Library
import paho.mqtt.client as mqtt


###############################################################################
# Logger Setup
###############################################################################

logging.basicConfig(
    #format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    format="%(message)s",
    level=logging.INFO
)

logger = logging.getLogger(__name__)


###############################################################################
# Constants - MQTT
###############################################################################

# MQTT Broker Setup
MQTT_HOST = "test.mosquitto.org"
MQTT_PORT = 1883

# Topic from Server to Setup Device:
# Trigger update check, Provide last FW version, FW update start message
MQTT_TOPIC_PUB_OTA_SETUP = "/{}/ota/setup"

# Topic from Server to send Firmware Data messages
MQTT_TOPIC_PUB_OTA_DATA = "/{}/ota/data"

# Topic from Device for OTA Control Requests:
# FW Update check (request last FW version), Request FW update,
# Notify Update completed
MQTT_TOPIC_SUB_OTA_CONTROL = "/{}/ota/control"

# Topic from Device to Acknowledge of received FW data blocks messages
MQTT_TOPIC_SUB_OTA_ACK = "/{}/ota/ack"


###############################################################################
# Constants - Control Message Commands (Device to Server)
###############################################################################

# Device request a FW Update Check to get last FW information from server
MSG_CONTROL_CMD_FW_UPDATE_CHECK = [ 0xaf, 0x12, 0x34, 0x56 ]

# Device request to launch a FUOTA process to Server
MSG_CONTROL_CMD_REQUEST_FW_UPDATE = [ 0x55, 0x55, 0xff, 0xff ]

# Device ready to start FUOTA process and handle reception of FW data blocks
MSG_ACK_FUOTA_START = [ 0xaa, 0xaa, 0xaa, 0xaa ]

# FUOTA process completed successfully
MSG_CONTROL_CMD_FW_UPDATE_COMPLETED_OK = [ 0x55, 0xaa, 0xff, 0xff ]

# FUOTA process completed but update on device has fail
MSG_CONTROL_CMD_FW_UPDATE_COMPLETED_FAIL = [ 0x55, 0xaa, 0x00, 0x00 ]


###############################################################################
# Constants - Setup Message Commands (Server to Device)
###############################################################################

MSG_SETUP_CMD_TRIGGER_FW_UPDATE_CHECK = [ 0x00 ]

MSG_SETUP_CMD_LAST_FW_INFO = [ 0x01 ]

# Start of FUOTA process message that provides all information of the Firmware
# data that is going to be sent (Firmware version, size and checksum)
MSG_SETUP_CMD_FUOTA_START = [ 0x02 ]

# Last Fw Version value to make the device to accept any kind of FW
FW_VER_MAJOR_FORCE_UPDATE = 0
FW_VER_MINOR_FORCE_UPDATE = 0
FW_VER_PATCH_FORCE_UPDATE = 0

# Firmware Data Block Size
FW_DATA_BLOCK_SIZE = 1024


###############################################################################
# Globals
###############################################################################

app_exit = False
mqtt_just_connected = False
mqtt_msg_rx_ota_control = False
mqtt_msg_rx_ota_ack = False
msg_rx_payload = None


###############################################################################
# Auxiliary Functions
###############################################################################

def file_read(file_path: str):
    '''Read full content of a binary file.'''
    read_bytes = None
    try:
        with open(file_path, "rb") as bin_file_reader:
            read_bytes = bin_file_reader.read()
    except Exception:
        logger.error(format_exc())
        logger.error(f"Fail to read binary file {file_path}\n")
    return read_bytes

def mqtt_publish_ota_setup(client, command):
    logger.info(f"({len(command)}) [ {command.hex()} ]")
    logger.info("")
    client.publish(MQTT_TOPIC_PUB_OTA_SETUP, command)

def mqtt_publish_ota_data(client, block_n, fw_data):
    msg_payload = bytearray()
    msg_payload += block_n.to_bytes(4, "big")
    msg_payload += fw_data[block_n]
    client.publish(MQTT_TOPIC_PUB_OTA_DATA, msg_payload)

def mqtt_force_device_check_for_update(client):
    '''Send MQTT msg to device to make it trigger a FW Update Check.'''
    msg_payload = bytearray(MSG_SETUP_CMD_TRIGGER_FW_UPDATE_CHECK)
    logger.info("Sending force check for FW Update "
                "(MSG_SETUP_CMD_TRIGGER_FW_UPDATE_CHECK)")
    mqtt_publish_ota_setup(client, msg_payload)

def mqtt_send_last_fw_info(client, fw_size, fw_md5):
    '''Send MQTT msg to device to make it trigger a FW Update Check.'''
    msg_payload = bytearray(MSG_SETUP_CMD_LAST_FW_INFO)
    msg_payload += FW_VER_MAJOR_FORCE_UPDATE.to_bytes(1, "big")
    msg_payload += FW_VER_MINOR_FORCE_UPDATE.to_bytes(1, "big")
    msg_payload += FW_VER_PATCH_FORCE_UPDATE.to_bytes(1, "big")
    msg_payload += fw_size.to_bytes(4, "big")
    msg_payload +=  bytearray(fw_md5.hex().encode())
    logger.info("Sending last FW info (MSG_SETUP_CMD_LAST_FW_INFO)")
    mqtt_publish_ota_setup(client, msg_payload)

def mqtt_send_fuota_start(client):
    '''Send MQTT msg to device to start the FUOTA process.'''
    msg_payload = bytearray(MSG_SETUP_CMD_FUOTA_START)
    logger.info("Sending FUOTA process start (MSG_SETUP_CMD_FUOTA_START)")
    mqtt_publish_ota_setup(client, msg_payload)


###############################################################################
# MQTT Callback Functions
###############################################################################

def cb_mqtt_on_connect(client, userdata, flags, rc):
    global mqtt_just_connected
    logger.info("MQTT connected to Broker")
    client.subscribe(MQTT_TOPIC_SUB_OTA_CONTROL, qos=2)
    client.subscribe(MQTT_TOPIC_SUB_OTA_ACK, qos=2)
    mqtt_just_connected = True

def cb_mqtt_on_msg_rx(client, userdata, msg):
    global mqtt_msg_rx_ota_control
    global mqtt_msg_rx_ota_ack
    global msg_rx_payload
    msg_rx_payload = None
    if msg.topic == MQTT_TOPIC_SUB_OTA_CONTROL:
        msg_rx_payload = msg.payload
        mqtt_msg_rx_ota_control = True
    elif msg.topic == MQTT_TOPIC_SUB_OTA_ACK:
        msg_rx_payload = msg.payload
        mqtt_msg_rx_ota_ack = True
    else:
        logger.warning("Msg rx on unexpected topic")


###############################################################################
# MQTT Process Thread (Required to publish msg on callbacks)
###############################################################################

def th_mqtt_process(client):
    client.loop_forever()


###############################################################################
# Over The Air Management
###############################################################################

def manage_ota(device_id, fw_file_path):
    global MQTT_TOPIC_PUB_OTA_SETUP
    global MQTT_TOPIC_PUB_OTA_DATA
    global MQTT_TOPIC_SUB_OTA_CONTROL
    global MQTT_TOPIC_SUB_OTA_ACK
    global mqtt_just_connected
    global mqtt_msg_rx_ota_control
    global mqtt_msg_rx_ota_ack
    update_success = False
    # Prepare MQTT Topics to use (add device ID to them)
    MQTT_TOPIC_PUB_OTA_SETUP = MQTT_TOPIC_PUB_OTA_SETUP.format(device_id)
    MQTT_TOPIC_PUB_OTA_DATA = MQTT_TOPIC_PUB_OTA_DATA.format(device_id)
    MQTT_TOPIC_SUB_OTA_CONTROL = MQTT_TOPIC_SUB_OTA_CONTROL.format(device_id)
    MQTT_TOPIC_SUB_OTA_ACK = MQTT_TOPIC_SUB_OTA_ACK.format(device_id)
    logger.info(f"MQTT_TOPIC_PUB_OTA_SETUP: {MQTT_TOPIC_PUB_OTA_SETUP}")
    logger.info(f"MQTT_TOPIC_PUB_OTA_DATA: {MQTT_TOPIC_PUB_OTA_DATA}")
    logger.info(f"MQTT_TOPIC_SUB_OTA_CONTROL: {MQTT_TOPIC_SUB_OTA_CONTROL}")
    logger.info(f"MQTT_TOPIC_SUB_OTA_ACK: {MQTT_TOPIC_SUB_OTA_ACK}")
    # Read Firmware file and calculate size and number of data blocks
    fw_data = file_read(fw_file_path)
    if fw_data is None:
        return False
    fw_data_md5 = hashlib.md5(fw_data).digest()
    fw_size = len(fw_data)
    num_blocks = int(fw_size / FW_DATA_BLOCK_SIZE)
    last_block_size = fw_size % FW_DATA_BLOCK_SIZE
    if last_block_size != 0:
        num_blocks = num_blocks + 1
    logger.info(f"Firmware: {fw_file_path}")
    logger.info(f"Firmware MD5: {fw_data_md5.hex()}")
    logger.info(f"Firmware size: {fw_size}")
    # Prepare FW data blocks
    list_fw_blocks = []
    block_start = 0
    block_end = FW_DATA_BLOCK_SIZE
    for _ in range(num_blocks - 1):
        list_fw_blocks.append(bytearray(fw_data[block_start:block_end]))
        block_start = block_start + FW_DATA_BLOCK_SIZE
        block_end = block_end + FW_DATA_BLOCK_SIZE
    if last_block_size != 0:
        list_fw_blocks.append(
            bytearray(fw_data[block_start:block_start+last_block_size]))
    logger.info(f"Number of blocks: {len(list_fw_blocks)}")
    logger.info(f"Last block size: {last_block_size}")
    # Launch MQTT Connection
    logger.info("Launching MQTT Connection...")
    mqtt_client = mqtt.Client()
    mqtt_client.on_connect = cb_mqtt_on_connect
    mqtt_client.on_message = cb_mqtt_on_msg_rx
    mqtt_client.connect(MQTT_HOST, MQTT_PORT, 60)
    th_mqtt_process_id = threading.Thread(
        target=th_mqtt_process,
        args=(mqtt_client,))
    th_mqtt_process_id.start()
    # Manage OTA Procedure
    block_n = 0
    logger.info("OTA Procedure Started")
    while not app_exit:
        # Once MQTT is connected
        # Force device to trigger a FW Update Check
        if mqtt_just_connected:
            mqtt_just_connected = False
            mqtt_force_device_check_for_update(mqtt_client)
        elif mqtt_msg_rx_ota_control:
            mqtt_msg_rx_ota_control = False
            if msg_rx_payload is None:
                continue
            cmd = list(msg_rx_payload)
            # Device Request check FW Update (get last FW information)
            if cmd == MSG_CONTROL_CMD_FW_UPDATE_CHECK:
                logger.info("Device request last available FW information")
                mqtt_send_last_fw_info(mqtt_client, fw_size, fw_data_md5)
            # Device request to launch a FUOTA process to Server
            elif cmd == MSG_CONTROL_CMD_REQUEST_FW_UPDATE:
                logger.info("Device request a FW Update")
                mqtt_send_fuota_start(mqtt_client)
            # Device ready to start FUOTA process
            elif cmd == MSG_ACK_FUOTA_START:
                # Send first FW data block
                block_n = 0
                logger.info(f"Sending FW block {block_n}")
                mqtt_publish_ota_data(mqtt_client, block_n, list_fw_blocks)
                block_n = block_n + 1
            # FUOTA process completed successfully
            elif cmd == MSG_CONTROL_CMD_FW_UPDATE_COMPLETED_OK:
                logger.info("Device notify Firmware Update completed")
                update_success = True
                break
            # FUOTA process completed but update on device has fail
            elif cmd == MSG_CONTROL_CMD_FW_UPDATE_COMPLETED_FAIL:
                logger.info("Device notify Firmware Update fail")
                break
            else:
                logger.warning("Unkown command received from Device")
        elif mqtt_msg_rx_ota_ack:
            mqtt_msg_rx_ota_ack = False
            if msg_rx_payload is None:
                continue
            ack_block_n = int.from_bytes(msg_rx_payload, "big")
            if ack_block_n != (block_n - 1):
                logger.error("Received ACK of unexpected FW block")
                logger.error(f"Expected {block_n}, received {ack_block_n}")
                break
            if block_n < num_blocks:
                logger.info(f"Sending FW block {block_n}")
                mqtt_publish_ota_data(mqtt_client, block_n, list_fw_blocks)
                block_n = block_n + 1
    # Close MQTT and wait for process thread end
    logger.info("Disconnecting from MQTT")
    mqtt_client.disconnect()
    mqtt_client.loop_stop()
    if th_mqtt_process_id.is_alive():
        th_mqtt_process_id.join()
    logger.info("MQTT Closed")
    return update_success


###############################################################################
# Arguments Parser
###############################################################################

def parse_options():
    '''Get and parse program input arguments.'''
    parser = ArgumentParser()
    parser.version = VERSION
    parser.add_argument("-v", "--version", action="version")
    parser.add_argument("--device", action="store", type=str,
                        help="Specify the device ID (MAC) to update.")
    parser.add_argument("--firmware", action="store", type=str,
                        help="Specify the firmware application binary "
                             "file to send through OTA-MQTT.")
    args = parser.parse_args()
    return args


###############################################################################
# Main Function
###############################################################################

def main(argc, argv):
    args = parse_options()
    if args.device and args.firmware:
        if not manage_ota(args.device, args.firmware):
            return 1
    return 0


###############################################################################
# System Termination Signals Management
###############################################################################

def system_termination_signal_handler(signal,  frame):
    '''Termination signals detection handler. It stop application execution.'''
    global app_exit
    app_exit = True


def system_termination_signal_setup():
    '''
    Attachment of System termination signals (SIGINT, SIGTERM, SIGUSR1) to
    function handler.
    '''
    # SIGTERM (kill pid) to signal_handler
    signal(SIGTERM, system_termination_signal_handler)
    # SIGINT (Ctrl+C) to signal_handler
    signal(SIGINT, system_termination_signal_handler)
    # SIGUSR1 (self-send) to signal_handler
    if os_system() != "Windows":
        signal(SIGUSR1, system_termination_signal_handler)


###############################################################################
# Runnable Main Script Detection
###############################################################################

if __name__ == '__main__':
    logger.info("{} v{} {}\n".format(os_path.basename(NAME), VERSION, DATE))
    system_termination_signal_setup()
    return_code = main(len(sys_argv) - 1, sys_argv[1:])
    logger.info(f"Exit ({return_code})")
    sys_exit(return_code)
