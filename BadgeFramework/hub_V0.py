from __future__ import division, absolute_import, print_function
import logging
import sys
import threading

from badge import *
from ble_badge_connection import *
from bluepy import *
from bluepy import btle
from bluepy.btle import UUID, Peripheral, DefaultDelegate, AssignedNumbers ,Scanner
from bluepy.btle import BTLEException
import numpy as np
import pandas as pd

class Connection:

    def __init__(self,address):
        #print("Connecting to badge", device_addr)	
        self.connection = BLEBadgeConnection.get_connection_to_badge(address)
        if not self.connection:
            print("Could not connect")
        self.connection.connect()
        self.badge = OpenBadge(self.connection) 

    def handle_status_request(self,args):
        if len(args) == 0:
            print(self.badge.get_status())
        elif len(args) == 1:
            print("Badge ID and Group Number Must Be Set Simultaneously")
        elif len(args) == 2:
            new_id = int(args[0])
            group_number = int(args[1])
            print(self.badge.get_status(new_id=new_id, new_group_number=group_number))
        else:
            print("Invalid Syntax: status [new badge id] [group number]")

    def handle_start_microphone_request(self,args):
        print(self.badge.start_microphone())

    def handle_stop_microphone_request(self,args):
        self.badge.stop_microphone()

    def handle_start_scan_request(self,args):
        print(self.badge.start_scan())

    def handle_stop_scan_request(self,args):
        self.badge.stop_scan()
            
    def handle_start_imu_request(self,args):
        print(self.badge.start_imu())

    def handle_stop_imu_request(self,args):
        self.badge.stop_imu()
    
    def handle_identify_request(self,args):
        if len(args) == 1:
            self.badge.identify()
        elif len(args) == 2:
            if args[1] == "off":
                self.badge.identify(duration_seconds=0)
            else:
                self.badge.identify(duration_seconds=int(args[1]))
        else:
            print("Invalid Syntax: identify [led duration seconds | 'off']")
            return

    def handle_restart_request(self,args):
        print(self.badge.restart())

    def handle_get_free_space(self,args):
        print(self.badge.get_free_sdc_space())
    
    command_handlers = {
        "status": handle_status_request,
        "start_microphone": handle_start_microphone_request,
        "stop_microphone": handle_stop_microphone_request,
        "start_scan": handle_start_scan_request,
        "stop_scan": handle_stop_scan_request,
        "start_imu": handle_start_imu_request,
        "stop_imu": handle_stop_imu_request,
        "identify": handle_identify_request,
        "restart": handle_restart_request,
        "get_free_space": handle_get_free_space,
    }
def start_recording_all_devices():

def stop_recording_all_devices():

def synchronise_all_devices():



if __name__ == "__main__":

    df = pd.read_csv('sample_mapping_file.csv')
    device_addr = sys.argv[1]
    cur_connection=Connection(device_addr)
    cur_connection.handle_status_request({})


