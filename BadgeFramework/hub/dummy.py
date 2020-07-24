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

    def __init__(self, pid, address, executor=None):
        self.badge_id = int(pid)
        self.mac_address = address
        self.executor = executor
        self.sync_status = False
        self.status = None
        self.widgets_dict = None
        self.widget = None

        # first connection with the device
        try:
            self.connect()
            self.disconnect()
        except Exception as ex:
            logger.warning("Error connecting with MIDGE " +
                           str(pid) + ": " + str(ex))

    @staticmethod
    def from_mapping_file(fpath, executor=None):
        df = pd.read_csv(fpath)
        connections = list()
        for _, row in df.iterrows():
            current_participant = row["Participant Id"]
            current_mac = row["Mac Address"]
            cur_connection = DummyConnection(
                current_participant, current_mac, executor)

            connections.append(cur_connection)
        return connections

    def connect(self):
        time.sleep(DummyConnection.conn_delay)

    def start(self):
        self.connect()
        self.set_id_at_start()
        self.start_recording_all_sensors()
        self.disconnect()

    def set_id_at_start(self):
        p = random.random()
        time.sleep(DummyConnection.req_delay)
        if p < 0.2:
            raise Exception(
                "Could not set id for participant " + str(self.badge_id))

    def disconnect(self):
        time.sleep(DummyConnection.conn_delay)

    def get_status_future(self):
        future = self.executor.submit(self.handle_status_request)
        return future

    def status(self):
        return self.get_status_future.result()

    def start_microphone(self):
        if self.executor:
            return self.executor.submit(self.handle_start_microphone_request).result()
        else:
            return self.handle_start_microphone_request()

    def stop_microphone(self):
        if self.executor:
            return self.executor.submit(self.handle_stop_microphone_request).result()
        else:
            return self.handle_stop_microphone_request()

    def start_imu(self):
        if self.executor:
            return self.executor.submit(self.handle_start_imu_request).result()
        else:
            return self.handle_start_imu_request()
            

    def stop_imu(self):
        if self.executor:
            return self.executor.submit(self.handle_stop_imu_request).result()
        else:
            return self.handle_stop_imu_request()

    def start_scan(self):
        if self.executor:
            return self.executor.submit(self.handle_start_scan_request).result()
        else:
            return self.handle_start_scan_request()

    def stop_scan(self):
        if self.executor:
            return self.executor.submit(self.handle_stop_scan_request).result()
        else:
            return self.handle_stop_scan_request()

    def handle_status_request(self):
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

    def handle_start_microphone_request(self):
        p = random.random()
        time.sleep(DummyConnection.req_delay)
        if p < 0.1:
            raise Exception(
                "Could not get status for participant " + str(self.badge_id))
        else:
            res = StartMicrophoneResponse()
            return res

    def handle_stop_microphone_request(self):
        p = random.random()
        time.sleep(DummyConnection.req_delay)
        if p < 0.1:
            raise Exception(
                "Could not stop mic for participant " + str(self.badge_id))

    def handle_start_scan_request(self):
        p = random.random()
        time.sleep(DummyConnection.req_delay)
        if p < 0.1:
            raise Exception(
                "Could not start scan for participant " + str(self.badge_id))
        else:
            res = StartScanResponse()
            return res

    def handle_stop_scan_request(self):
        p = random.random()
        time.sleep(DummyConnection.req_delay)
        if p < 0.1:
            raise Exception(
                "Could not stop scan for participant " + str(self.badge_id))

    def handle_start_imu_request(self):
        p = random.random()
        time.sleep(DummyConnection.req_delay)
        if p < 0.1:
            raise Exception(
                "Could not start IMU for participant " + str(self.badge_id))
        else:
            res = StartImuResponse()
            return res

    def handle_stop_imu_request(self):
        p = random.random()
        time.sleep(DummyConnection.req_delay)
        if p < 0.1:
            raise Exception(
                "Could not stop IMU for participant " + str(self.badge_id))

    def handle_identify_request(self):
        p = random.random()
        time.sleep(DummyConnection.req_delay)
        if p < 0.1:
            raise Exception(
                "Could not identify for participant " + str(self.badge_id))
        else:
            res = StatusResponse()
            return res

    def handle_restart_request(self):
        p = random.random()
        time.sleep(DummyConnection.req_delay)
        if p < 0.1:
            raise Exception(
                "Could not restart for participant " + str(self.badge_id))
        else:
            res = StatusResponse()
            return res

    def handle_get_free_space(self):
        p = random.random()
        time.sleep(DummyConnection.req_delay)
        if p < 0.1:
            raise Exception(
                "Could not get free space for participant " + str(self.badge_id))
        else:
            res = StatusResponse()
            return res

    def start_recording_all_sensors(self):
        self.handle_status_request()
        self.handle_start_scan_request()
        self.handle_start_microphone_request()
        self.handle_start_imu_request()

    def stop_recording_all_sensors(self):
        self.handle_stop_scan_request()
        self.handle_stop_microphone_request()
        self.handle_stop_imu_request()

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
