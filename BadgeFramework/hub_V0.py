from __future__ import division, absolute_import, print_function
import logging
import sys
import threading

from badge import OpenBadge
from ble_badge_connection import BLEBadgeConnection
#from bluepy import *
from bluepy import btle
from bluepy.btle import UUID, Peripheral, DefaultDelegate, AssignedNumbers ,Scanner
from bluepy.btle import BTLEException
import numpy as np
import pandas as pd
import time
import signal,tty,termios

constant_group_number = 1

class Connection():
    def __init__(self,pid,address):
        try:
            self.connection = BLEBadgeConnection.get_connection_to_badge(address)
            self.connection.connect()
            self.badge = OpenBadge(self.connection)
            self.badge_id = int(pid)
            self.mac_address = address
            self.group_number = int(constant_group_number)
            print ("MY ID:" + str(self.badge_id))
        except:
            raise Exception("Could not connect to participant" + str(pid))

    def set_id_at_start(self):
        try:
            self.badge.get_status(new_id=self.badge_id, new_group_number=self.group_number)
        except Exception as err:
            print (err)

    def disconnect(self):
        self.connection.disconnect()

    def handle_status_request(self):
        try:
            out = self.badge.get_status()
            return out
        except Exception as err:
            print (str(err))
            raise Exception("Could not get status for participant")

    def handle_start_microphone_request(self):
        try:
            self.badge.start_microphone()
        except:
            raise Exception("Could not start mic for participant" + str(self.badge_id))

    def handle_stop_microphone_request(self):
        try:
            self.badge.stop_microphone()
        except:
            raise Exception("Could not stop mic for participant" + str(self.badge_id))

    def handle_start_scan_request(self):
        try:
            self.badge.start_scan()
        except:
            raise Exception("Could not start scan for participant" + str(self.badge_id))

    def handle_stop_scan_request(self):
        try:
            self.badge.stop_scan()
        except:
            raise Exception("Could not stop scan for participant" + str(self.badge_id))

    def handle_start_imu_request(self):
        try:
            self.badge.start_imu()
        except:
            raise Exception("Could not start IMU for participant" + str(self.badge_id))

    def handle_stop_imu_request(self):
        try:
            self.badge.stop_imu()
        except:
            raise Exception("Could not stop IMU for participant " + str(self.badge_id))

    def handle_identify_request(self):
        try:
            self.badge.identify()
        except:
            raise Exception("Could not identify for participant " + str(self.badge_id))

    def handle_restart_request(self):
        try:
            self.badge.restart()
        except:
            raise Exception("Could not restart for participant " + str(self.badge_id))

    def handle_get_free_space(self):
        try:
            print(self.badge.get_free_sdc_space())
        except:
            raise Exception("Could not get free space for participant " + str(self.badge_id))

    def start_recording_all_sensors(self):
        self.handle_status_request()
        self.handle_start_scan_request()
        self.handle_start_microphone_request()
        self.handle_start_imu_request()

    def stop_recording_all_sensors(self):
        self.handle_stop_scan_request()
        self.handle_stop_microphone_request()
        self.handle_stop_imu_request()

    def print_help(self):
        print("> Available commands:")
        print("> status ")
        print("> start_all_sensors")
        print("> stop_all_sensors")
        print("> start_microphone")
        print("> stop_microphone")
        print("> start_scan")
        print("> stop_scan")
        print("> start_imu")
        print("> stop_imu")
        print("> identify [led duration seconds | 'off']")
        print("> restart")
        print("> get_free_space")
        print("> help")
        print("> All commands use current system time as transmitted time.")

##################################################################################
##################################################################################

def choose_function(connection,input):
    chooser = {
        "help": connection.print_help,
        "status": connection.handle_status_request,
        "start_all_sensors": connection.start_recording_all_sensors,
        "stop_all_sensors": connection.stop_recording_all_sensors,
        "start_microphone": connection.handle_start_microphone_request,
        "stop_microphone": connection.handle_stop_microphone_request,
        "start_scan": connection.handle_start_scan_request,
        "stop_scan": connection.handle_stop_scan_request,
        "start_imu": connection.handle_start_imu_request,
        "stop_imu": connection.handle_stop_imu_request,
        "identify": connection.handle_identify_request,
        "restart": connection.handle_restart_request,
        "get_free_space": connection.handle_get_free_space,
    }
    func = chooser.get(input, lambda: "Invalid month")
    out = func()
    return out


def start_recording_all_devices(df):
    for _, row in df.iterrows():
        current_participant = row['Participant Id']
        current_mac = row['Mac Address']
        try:
            cur_connection=Connection(current_participant,current_mac)
        except Exception as error:
            print(str(error) + ', sensors are not started.')
            continue
        try:
            cur_connection.set_id_at_start()
            cur_connection.start_recording_all_sensors()
            cur_connection.disconnect()
        except Exception as error:
            print(error)
            cur_connection.disconnect()

def stop_recording_all_devices(df):
    for _, row in df.iterrows():
        current_participant = row['Participant Id']
        current_mac = row['Mac Address']
        try:
            cur_connection=Connection(current_participant,current_mac)
        except Exception as error:
            print(str(error) + ', sensors are not stopped.')
            continue
        try:
            cur_connection.stop_recording_all_sensors()
            cur_connection.disconnect()
        except Exception as error:
            print(str(error))
            cur_connection.disconnect()

def synchronise_and_check_all_devices(df):
    for _, row in df.iterrows():
        current_participant = row['Participant Id']
        current_mac = row['Mac Address']
        try:
            cur_connection=Connection(current_participant,current_mac)
        except Exception as error:
            print(str(error) + ', cannot synchronise.')
            continue
        try:
            out = cur_connection.handle_status_request()
            if out.imu_status == 0:
                print ('IMU is not recording for participant ' + str(current_participant))
            if out.microphone_status == 0:
                print ('Mic is not recording for participant ' + str(current_participant))
            if out.scan_status == 0:
                print ('Scan is not recording for participant ' + str(current_participant))
            if out.clock_status == 0:
                print ('Cant synch for participant ' + str(current_participant))
            cur_connection.disconnect()
        except Exception as error:
            print(error)
            cur_connection.disconnect()

class TimeoutInput(object):
    def __init__(self, poll_period=0.05):
        self.poll_period = poll_period

    def _getch_nix(self):
        from select import select
        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setraw(sys.stdin.fileno())
            [i, _, _] = select([sys.stdin.fileno()], [], [], self.poll_period)
            if i:
                ch = sys.stdin.read(1)
            else:
                ch = ''
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        return ch

    def input(self, prompt=None, timeout=None,
              extend_timeout_with_input=True, require_enter_to_confirm=True):
        """timeout: float seconds or None (blocking)"""
        prompt = prompt or ''
        sys.stdout.write(prompt)
        sys.stdout.flush()
        input_chars = []
        start_time = time.time()
        received_enter = False
        while (time.time() - start_time) < timeout:
            c = self._getch_nix()
            if c in ('\n', '\r'):
                received_enter = True
                break
            elif c:
                input_chars.append(c)
                sys.stdout.write(c)
                sys.stdout.flush()
                if extend_timeout_with_input:
                    start_time = time.time()
        sys.stdout.write('\n')
        sys.stdout.flush()
        captured_string = ''.join(input_chars)
        if require_enter_to_confirm:
            return_string = captured_string if received_enter else ''
        else:
            return_string = captured_string
        return return_string

if __name__ == "__main__":
    df = pd.read_csv('sample_mapping_file.csv')
    while True:
        print("> Type start to start data collection or stop to finish data collection.")
        sys.stdout.write("> ")
        command = sys.stdin.readline()[:-1]
        if command == "start":
            start_recording_all_devices(df)
            while True:
                ti = TimeoutInput(poll_period=0.05)
                s = ti.input(prompt='>Type int if you would like to enter interactive shell.\n'+'>', timeout=10.0,
                            extend_timeout_with_input=False, require_enter_to_confirm=True)
                if s == "int":
                    print("> Welcome to the interactive shell. Please type the id of the Midge you want to connect.")
                    print("> Type exit if you would like to stop recording for all devices.")
                    sys.stdout.write("> ")
                    command = sys.stdin.readline()[:-1]
                    if command == "exit":
                        print("> Stopping the recording of all devices.")
                        stop_recording_all_devices(df)
                        print("> Devices are stopped.")
                        break
                    command_args = command.split(" ")
                    current_mac_addr= (df.loc[df['Participant Id'] == int(command)]['Mac Address']).values[0]
                    try:
                        cur_connection = Connection(int(command),current_mac_addr)
                    except Exception as error:
                        print (str(error))
                        continue
                    print ('> Connected to the badge. For available commands, please type help.')
                    print ('> ')
                    while True:
                        command = sys.stdin.readline()[:-1]
                        command_args = command.split(" ")
                        if command == "exit":
                            cur_connection.disconnect()
                            break
                        if command_args[0] in cur_connection.command_handlers:
                            try:
                                out = choose_function(cur_connection,command_args[0])
                                print (out)
                            except Exception as error:
                                print (str(error))
                                continue
                        else:
                            print("> Command not found!")
                            cur_connection.print_help()
                else:
                    print('> Synchronisation is starting. Please wait till ends.')
                    synchronise_and_check_all_devices(df)
                    print('> Synchronisation is finished.')
        elif command == "stop":
            print("> Stopping data collection.")
            quit(0)
        else:
            print("> Command not found, please type start or stop to start or stop data collection.")

