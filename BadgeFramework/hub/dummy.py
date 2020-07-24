import sys
import random
import time
import logging
sys.path.append('..')

import pandas as pd
import ipywidgets as widgets

from badge_protocol import StatusResponse, StartMicrophoneResponse, StartScanResponse, StartImuResponse, FreeSDCSpaceResponse

constant_group_number = 1
logger = logging.getLogger(__name__)

class DummyConnection:
    conn_delay = 0.1
    req_delay = 0.1

    def __init__(self, pid, address):
        self.badge_id = int(pid)
        self.mac_address = address
        self.sync_status = False
        self.status = None
        self.widgets_dict = None
        self.widget = None

    @staticmethod
    def from_mapping_file(fpath):
        df = pd.read_csv(fpath)
        connections = list()
        for _, row in df.iterrows():
            current_participant = row["Participant Id"]
            current_mac = row["Mac Address"]
            cur_connection = DummyConnection(
                current_participant, current_mac)

            connections.append(cur_connection)
        return connections

    def connect(self):
        time.sleep(DummyConnection.conn_delay)

    def start(self):
        try:
            self.connect()
            self.set_id_at_start()
            self.get_status()
            self.start_scan()
            self.start_microphone()
            self.start_imu()
            self.disconnect()
        except Exception as e:
            print 'Could not start to MIDGE {:d}'.format(self.badge_id)
            print str(e)

    def stop_all_sensors(self):
        self.stop_scan()
        self.stop_microphone()
        self.stop_imu()

    def set_id_at_start(self):
        p = random.random()
        time.sleep(DummyConnection.req_delay)
        if p < 0.2:
            raise Exception(
                "Could not set id for participant " + str(self.badge_id))

    def disconnect(self):
        time.sleep(DummyConnection.conn_delay)

    def get_status(self):
        p = random.random()
        time.sleep(DummyConnection.req_delay)
        if p < 0.1:
            self.update_status(False)
            raise Exception(
                "Could not get status for participant " + str(self.badge_id))
        res = StatusResponse()
        res.imu_status = int(round(random.random()))
        res.microphone_status = int(round(random.random()))
        res.scan_status = int(round(random.random()))
        self.update_status(True, res)
        return res

    def start_microphone(self):
        p = random.random()
        time.sleep(DummyConnection.req_delay)
        if p < 0.1:
            raise Exception(
                "Could not get status for participant " + str(self.badge_id))
        else:
            res = StartMicrophoneResponse()
            return res

    def stop_microphone(self):
        p = random.random()
        time.sleep(DummyConnection.req_delay)
        if p < 0.1:
            raise Exception(
                "Could not stop mic for participant " + str(self.badge_id))

    def start_scan(self):
        p = random.random()
        time.sleep(DummyConnection.req_delay)
        if p < 0.1:
            raise Exception(
                "Could not start scan for participant " + str(self.badge_id))
        else:
            res = StartScanResponse()
            return res

    def stop_scan(self):
        p = random.random()
        time.sleep(DummyConnection.req_delay)
        if p < 0.1:
            raise Exception(
                "Could not stop scan for participant " + str(self.badge_id))

    def start_imu(self):
        p = random.random()
        time.sleep(DummyConnection.req_delay)
        if p < 0.1:
            raise Exception(
                "Could not start IMU for participant " + str(self.badge_id))
        else:
            res = StartImuResponse()
            return res

    def stop_imu(self):
        p = random.random()
        time.sleep(DummyConnection.req_delay)
        if p < 0.1:
            raise Exception(
                "Could not stop IMU for participant " + str(self.badge_id))

    def identify(self):
        p = random.random()
        time.sleep(DummyConnection.req_delay)
        if p < 0.1:
            raise Exception(
                "Could not identify for participant " + str(self.badge_id))
        else:
            res = StatusResponse()
            return res

    def restart(self):
        p = random.random()
        time.sleep(DummyConnection.req_delay)
        if p < 0.1:
            raise Exception(
                "Could not restart for participant " + str(self.badge_id))
        else:
            res = StatusResponse()
            return res

    def get_free_space(self):
        p = random.random()
        time.sleep(DummyConnection.req_delay)
        if p < 0.1:
            raise Exception(
                "Could not get free space for participant " + str(self.badge_id))
        else:
            res = StatusResponse()
            return res

    def update_status(self, sync_status, status=None):
        self.sync_status = sync_status

        if sync_status:
            self.status = status
            if self.widget is not None:
                self.widgets_dict['sync'].button_style = 'success'

                if self.status.microphone_status == 0:
                    self.widgets_dict['mic'].button_style = 'danger'
                else:
                    self.widgets_dict['mic'].button_style = 'success'
    
                if self.status.imu_status == 0:
                    self.widgets_dict['imu'].button_style = 'danger'
                else:
                    self.widgets_dict['imu'].button_style = 'success'

                if self.status.scan_status == 0:
                    self.widgets_dict['scan'].button_style = 'danger'
                else:
                    self.widgets_dict['scan'].button_style = 'success'
        else:
            if self.widget is not None:
                self.widgets_dict['sync'].button_style = 'info'
                self.widgets_dict['mic'].button_style = 'info'
                self.widgets_dict['imu'].button_style = 'info'
                self.widgets_dict['scan'].button_style = 'info'

    def create_widget(self):
        # put the buttons in a dict, just for ease of access in update_status
        self.widgets_dict = {
            'sync': widgets.ToggleButton(
                description='SYNC',
                disabled=False,
                button_style='success',  # 'success', 'info', 'warning', 'danger' or ''
                icon='check'
            ),
            'mic': widgets.ToggleButton(
                value=False,
                description='MIC',
                disabled=False,
                button_style='success',  # 'success', 'info', 'warning', 'danger' or ''
                tooltip='Description',
                icon='check'
            ),
            'imu': widgets.ToggleButton(
                value=False,
                description='IMU',
                disabled=False,
                button_style='success',  # 'success', 'info', 'warning', 'danger' or ''
                tooltip='Description',
                icon='check'
            ),
            'scan': widgets.ToggleButton(
                value=False,
                description='SCAN',
                disabled=False,
                button_style='success',  # 'success', 'info', 'warning', 'danger' or ''
                tooltip='Description',
                icon='check'
            )
        }
        # but the buttons in a box widget
        self.widget = widgets.HBox(list(self.widgets_dict.values()))

    def show(self):
        if self.widget is None:
            self.create_widget()
            self.update_status(self.sync_status, self.status)

        return display(self.widget)
