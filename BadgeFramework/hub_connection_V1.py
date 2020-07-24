from badge import OpenBadge
from ble_badge_connection import BLEBadgeConnection
import sys
import logging
import ipywidgets as widgets

import pandas as pd

constant_group_number = 1
logger = logging.getLogger(__name__)

class Connection:
    def __init__(self, pid, address, executor=None):
        self.badge_id = int(pid)
        self.group_number = int(constant_group_number)
        self.mac_address = address
        self.executor = executor
        self.sync_status = False
        self.status = None
        self.widgets_dict = None
        self.widget = None

        self.connection = BLEBadgeConnection.get_connection_to_badge(
            address
        )
        self.badge = OpenBadge(self.connection)


    @staticmethod
    def from_mapping_file(fpath, executor=None):
        df = pd.read_csv(fpath)
        connections = list()
        for _, row in df.iterrows():
            current_participant = row["Participant Id"]
            current_mac = row["Mac Address"]
            cur_connection = Connection(
                current_participant, current_mac, executor)

            connections.append(cur_connection)
        return connections

    def connect(self):
        try:
            for x in range(0, 10):
                try:
                    self.connection.connect()
                    break
                except Exception as err:
                    if x == 9:
                        raise Exception(
                            "Could not connect to participant " +
                            str(self.badge_id) + ", error:"
                            + str(err)
                        )
                    break
        except Exception as err:
            raise Exception("Could not connect to participant " + str(self.badge_id) + ", error:"
                            + str(err))

    def disconnect(self):
        self.connection.disconnect()

    def start(self):
        self.connect()
        self.set_id_at_start()
        self.start_recording_all_sensors()
        self.disconnect()

    def set_id_at_start(self):
        try:
            self.badge.get_status(
                new_id=self.badge_id, new_group_number=self.group_number
            )
        except Exception as err:
            raise Exception("Could not set id, error:" + str(err))

    def get_status_future(self):
        future = self.executor.submit(self.handle_status_request)
        return future

    def get_status(self):
        if self.executor:
            return self.get_status_future.result()
        else:
            return self.handle_status_request()

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

    def start_recording_all_sensors(self):
        self.get_status()
        self.start_scan()
        self.start_microphone()
        self.start_imu()

    def stop_recording_all_sensors(self):
        self.stop_scan()
        self.stop_microphone()
        self.stop_imu()

    def handle_status_request(self):
        try:
            self.connect()
            out = self.badge.get_status()
            self.disconnect()
            self.update_status(True, out)
            return out
        except Exception as err:
            self.update_status(False)
            raise Exception("Could not get status for participant " + str(self.badge_id)
                            + " , error:" + str(err))

    def handle_start_microphone_request(self):
        try:
            self.connect()
            out = self.badge.start_microphone()
            self.disconnect()
            return out
        except Exception as err:
            raise Exception("Could not start mic for participant " + str(self.badge_id)
                            + " , error:" + str(err))

    def handle_stop_microphone_request(self):
        try:
            self.connect()
            self.badge.stop_microphone()
            self.disconnect()
        except Exception as err:
            raise Exception("Could not stop mic for participant " + str(self.badge_id)
                            + " , error:" + str(err))

    def handle_start_scan_request(self):
        try:
            self.connect()
            out = self.badge.start_scan()
            self.disconnect()
            return out
        except Exception as err:
            raise Exception("Could not start scan for participant " + str(self.badge_id)
                            + " , error:" + str(err))

    def handle_stop_scan_request(self):
        try:
            self.connect()
            self.badge.stop_scan()
            self.disconnect()
        except Exception as err:
            raise Exception("Could not stop scan for participant " + str(self.badge_id)
                            + " , error:" + str(err))

    def handle_start_imu_request(self):
        try:
            self.connect()
            out = self.badge.start_imu()
            self.disconnect()
            return out
        except Exception as err:
            raise Exception("Could not start IMU for participant " + str(self.badge_id)
                            + " , error:" + str(err))

    def handle_stop_imu_request(self):
        try:
            self.connect()
            self.badge.stop_imu()
            self.disconnect()
        except Exception as err:
            raise Exception("Could not stop IMU for participant " + str(self.badge_id)
                            + " , error:" + str(err))

    def handle_identify_request(self):
        try:
            self.connect()
            out = self.badge.identify()
            self.disconnect()
            return out
        except Exception as err:
            raise Exception("Could not identify for participant " + str(self.badge_id)
                            + " , error:" + str(err))

    def handle_restart_request(self):
        try:
            self.connect()
            out = self.badge.restart()
            print("Please wait at least 10 seconds to connect back to the device.")
            print("Don't forget to start the recording for the restarted badge.")
            return out
        except Exception as err:
            raise Exception("Could not restart for participant " + str(self.badge_id)
                            + " , error:" + str(err))

    def handle_get_free_space(self):
        try:
            self.connect()
            out = self.badge.get_free_sdc_space()
            self.disconnect()
            return out
        except Exception as err:
            raise Exception(
                "Could not get free space for participant " + str(self.badge_id)
                + " , error:" + str(err)
            )

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
