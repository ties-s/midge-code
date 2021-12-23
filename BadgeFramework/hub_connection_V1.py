from badge import OpenBadge
from ble_badge_connection import BLEBadgeConnection
import sys

constant_group_number = 1


class Connection:
    def __init__(self, pid, address):
        try:
            for x in range(0, 10):
                try:
                    self.connection = BLEBadgeConnection.get_connection_to_badge(
                        address
                    )
                    self.connection.connect()
                    self.badge = OpenBadge(self.connection)
                    self.badge_id = int(pid)
                    self.mac_address = address
                    self.group_number = int(constant_group_number)
                    break
                except Exception as err:
                    if x == 9:
                        raise Exception(
                            "Could not connect to participant " + str(pid) + ", error:"
                            + str(err)
                        )
                    break
        except Exception as err:
            raise Exception("Could not connect to participant " + str(pid) + ", error:"
                            + str(err))

    def set_id_at_start(self):
        try:
            self.badge.get_status(
                new_id=self.badge_id, new_group_number=self.group_number
            )
        except Exception as err:
            raise Exception("Could not set id, error:" + str(err))

    def disconnect(self):
        self.connection.disconnect()

    def handle_status_request(self):
        try:
            out = self.badge.get_status()
            return out
        except Exception as err:
            raise Exception("Could not get status for participant " + str(self.badge_id)
                            + " , error:" + str(err))

    def handle_start_microphone_request(self):
        try:
            out = self.badge.start_microphone()
            return out
        except Exception as err:
            raise Exception("Could not start mic for participant " + str(self.badge_id)
                            + " , error:" + str(err))

    def handle_stop_microphone_request(self):
        try:
            self.badge.stop_microphone()
        except Exception as err:
            raise Exception("Could not stop mic for participant " + str(self.badge_id)
                            + " , error:" + str(err))

    def handle_start_scan_request(self):
        try:
            out = self.badge.start_scan()
            return out
        except Exception as err:
            raise Exception("Could not start scan for participant " + str(self.badge_id)
                            + " , error:" + str(err))

    def handle_stop_scan_request(self):
        try:
            self.badge.stop_scan()
        except Exception as err:
            raise Exception("Could not stop scan for participant " + str(self.badge_id)
                            + " , error:" + str(err))

    def handle_start_imu_request(self):
        try:
            out = self.badge.start_imu()
            return out
        except Exception as err:
            raise Exception("Could not start IMU for participant " + str(self.badge_id)
                            + " , error:" + str(err))

    def handle_stop_imu_request(self):
        try:
            self.badge.stop_imu()
        except Exception as err:
            raise Exception("Could not stop IMU for participant " + str(self.badge_id)
                            + " , error:" + str(err))

    def handle_identify_request(self):
        try:
            out = self.badge.identify()
            return out
        except Exception as err:
            raise Exception("Could not identify for participant " + str(self.badge_id)
                            + " , error:" + str(err))

    def handle_restart_request(self):
        try:
            out = self.badge.restart()
            print("Please wait at least 10 seconds to connect back to the device.")
            print("Don't forget to start the recording for the restarted badge.")
            return out
        except Exception as err:
            raise Exception("Could not restart for participant " + str(self.badge_id)
                            + " , error:" + str(err))

    def handle_get_free_space(self):
        try:
            out = self.badge.get_free_sdc_space()
            return out
        except Exception as err:
            raise Exception(
                "Could not get free space for participant " + str(self.badge_id)
                + " , error:" + str(err)
            )

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
        print(" Available commands:")
        print(" status ")
        print(" start_all_sensors")
        print(" stop_all_sensors")
        print(" start_microphone")
        print(" stop_microphone")
        print(" start_scan")
        print(" stop_scan")
        print(" start_imu")
        print(" stop_imu")
        print(" identify")
        print(" restart")
        print(" get_free_space")
        print(" help")
        print(" All commands use current system time as transmitted time.")
        sys.stdout.flush()
