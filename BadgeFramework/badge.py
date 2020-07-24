from __future__ import division, absolute_import, print_function
import time
import logging
import sys
import struct
import Queue

DEFAULT_SCAN_WINDOW = 250
DEFAULT_SCAN_INTERVAL = 1000

DEFAULT_IMU_ACC_FSR = 4  # Valid ranges: 2, 4, 8, 16
DEFAULT_IMU_GYR_FSR = 1000  # Valid ranges: 250, 500, 1000, 2000
DEFAULT_IMU_DATARATE = 50

DEFAULT_MICROPHONE_MODE = 1	#Valid options: 0=Stereo, 1=Mono

from badge_protocol import *

logger = logging.getLogger(__name__)

# -- Helper methods used often in badge communication --

# We generally define timestamp_seconds to be in number of seconds since UTC epoch
# and timestamp_miliseconds to be the miliseconds portion of that UTC timestamp.

# Returns the current timestamp as two parts - seconds and milliseconds
def get_timestamps():
    return get_timestamps_from_time(time.time())


# Returns the given time as two parts - seconds and milliseconds
def get_timestamps_from_time(t):
    timestamp_seconds = int(t)
    timestamp_fraction_of_second = t - timestamp_seconds
    timestamp_ms = int(1000 * timestamp_fraction_of_second)
    return (timestamp_seconds, timestamp_ms)


# Convert badge timestamp representation to python representation
def timestamps_to_time(timestamp_seconds, timestamp_miliseconds):
    return float(timestamp_seconds) + (float(timestamp_miliseconds) / 1000.0)


# Represents an OpenBadge currently connected via the BadgeConnection 'connection'.
#    The 'connection' should already be connected when it is used to initialize this class.
# Implements methods that allow for interaction with that badge.
class OpenBadge(object):
    def __init__(self, connection):
        self.connection = connection
        self.status_response_queue = Queue.Queue()
        self.start_microphone_response_queue = Queue.Queue()
        self.start_scan_response_queue = Queue.Queue()
        self.start_imu_response_queue = Queue.Queue()
        self.free_sdc_space_response_queue = Queue.Queue()

    # Helper function to send a BadgeMessage `command_message` to a device, expecting a response
    # of class `response_type` that is a subclass of BadgeMessage, or None if no response is expected.
    def send_command(self, command_message, response_type):
        expected_response_length = response_type.length() if response_type else 0

        serialized_command = command_message.serialize_message()
        logger.debug(
            "Sending: {}, Raw: {}".format(
                command_message, serialized_command.encode("hex")
            )
        )
        serialized_response = self.connection.send(
            serialized_command, response_len=expected_response_length
        )

        if expected_response_length > 0:
            response = response_type.deserialize_message(serialized_response)
            logger.info("Recieved response {}".format(response))
            return response
        else:
            logger.info("No response expected, transmission successful.")
            return True

    def send_request(self, request_message):
        serialized_request = request_message.encode()

        # Adding length header:
        serialized_request_len = struct.pack("<H", len(serialized_request))
        serialized_request = serialized_request_len + serialized_request

        logger.debug(
            "Sending: {}, Raw: {}".format(
                request_message, serialized_request.encode("hex")
            )
        )

        self.connection.send(serialized_request, response_len=0)

    def receive_response(self):
        response_len = struct.unpack("<H", self.connection.await_data(2))[0]
        logger.debug("Wait response len: " + str(response_len))
        serialized_response = self.connection.await_data(response_len)

        response_message = Response.decode(serialized_response)

        queue_options = {
            Response_status_response_tag: self.status_response_queue,
            Response_start_microphone_response_tag: self.start_microphone_response_queue,
            Response_start_scan_response_tag: self.start_scan_response_queue,
            Response_start_imu_response_tag: self.start_imu_response_queue,
            Response_free_sdc_space_response_tag: self.free_sdc_space_response_queue,
        }
        response_options = {
            Response_status_response_tag: response_message.type.status_response,
            Response_start_microphone_response_tag: response_message.type.start_microphone_response,
            Response_start_scan_response_tag: response_message.type.start_scan_response,
            Response_start_imu_response_tag: response_message.type.start_imu_response,
            Response_free_sdc_space_response_tag: response_message.type.free_sdc_space_response,
        }
        queue_options[response_message.type.which].put(
            response_options[response_message.type.which]
        )

    # Sends a status request to this Badge.
    #   Optional fields new_id and new_group number will set the badge's id
    #     and group number. They must be sent together.
    # Returns a StatusResponse() representing badge's response.
    def get_status(self, t=None, new_id=None, new_group_number=None):
        if t is None:
            (timestamp_seconds, timestamp_ms) = get_timestamps()
        else:
            (timestamp_seconds, timestamp_ms) = get_timestamps_from_time(t)

        request = Request()
        request.type.which = Request_status_request_tag
        request.type.status_request = StatusRequest()
        request.type.status_request.timestamp = Timestamp()
        request.type.status_request.timestamp.seconds = timestamp_seconds
        request.type.status_request.timestamp.ms = timestamp_ms
        if not ((new_id is None) or (new_group_number is None)):
            request.type.status_request.badge_assignement = BadgeAssignement()
            request.type.status_request.badge_assignement.ID = new_id
            request.type.status_request.badge_assignement.group = new_group_number
            request.type.status_request.has_badge_assignement = True

        self.send_request(request)

        # Clear the queue before receiving
        with self.status_response_queue.mutex:
            self.status_response_queue.queue.clear()

        while self.status_response_queue.empty():
            self.receive_response()

        return self.status_response_queue.get()

    # Sends a request to the badge to start recording microphone data.
    # Returns a StartRecordResponse() representing the badges response.
    def start_microphone(self, t=None, mode=DEFAULT_MICROPHONE_MODE):
        if t is None:
            (timestamp_seconds, timestamp_ms) = get_timestamps()
        else:
            (timestamp_seconds, timestamp_ms) = get_timestamps_from_time(t)

        request = Request()
        request.type.which = Request_start_microphone_request_tag
        request.type.start_microphone_request = StartMicrophoneRequest()
        request.type.start_microphone_request.timestamp = Timestamp()
        request.type.start_microphone_request.timestamp.seconds = timestamp_seconds
        request.type.start_microphone_request.timestamp.ms = timestamp_ms
        request.type.start_microphone_request.mode = mode

        self.send_request(request)

        with self.start_microphone_response_queue.mutex:
            self.start_microphone_response_queue.queue.clear()

        while self.start_microphone_response_queue.empty():
            self.receive_response()

        return self.start_microphone_response_queue.get()

    # Sends a request to the badge to stop recording.
    # Returns True if request was successfuly sent.
    def stop_microphone(self):

        request = Request()
        request.type.which = Request_stop_microphone_request_tag
        request.type.stop_microphone_request = StopMicrophoneRequest()

        self.send_request(request)

    # Sends a request to the badge to start performing scans and collecting scan data.
    #   window_miliseconds and interval_miliseconds controls radio duty cycle during scanning (0 for firmware default)
    #     radio is active for [window_miliseconds] every [interval_miliseconds]
    # Returns a StartScanningResponse() representing the badge's response.
    def start_scan(
        self, t=None, window_ms=DEFAULT_SCAN_WINDOW, interval_ms=DEFAULT_SCAN_INTERVAL
    ):
        if t is None:
            (timestamp_seconds, timestamp_ms) = get_timestamps()
        else:
            (timestamp_seconds, timestamp_ms) = get_timestamps_from_time(t)

        request = Request()
        request.type.which = Request_start_scan_request_tag
        request.type.start_scan_request = StartScanRequest()
        request.type.start_scan_request.timestamp = Timestamp()
        request.type.start_scan_request.timestamp.seconds = timestamp_seconds
        request.type.start_scan_request.timestamp.ms = timestamp_ms
        request.type.start_scan_request.window = window_ms
        request.type.start_scan_request.interval = interval_ms

        self.send_request(request)

        # Clear the queue before receiving
        with self.start_scan_response_queue.mutex:
            self.start_scan_response_queue.queue.clear()

        while self.start_scan_response_queue.empty():
            self.receive_response()

        return self.start_scan_response_queue.get()

    # Sends a request to the badge to stop scanning.
    # Returns True if request was successfuly sent.
    def stop_scan(self):

        request = Request()
        request.type.which = Request_stop_scan_request_tag
        request.type.stop_scan_request = StopScanRequest()

        self.send_request(request)

    def start_imu(
        self,
        t=None,
        acc_fsr=DEFAULT_IMU_ACC_FSR,
        gyr_fsr=DEFAULT_IMU_GYR_FSR,
        datarate=DEFAULT_IMU_DATARATE,
    ):
        if t is None:
            (timestamp_seconds, timestamp_ms) = get_timestamps()
        else:
            (timestamp_seconds, timestamp_ms) = get_timestamps_from_time(t)

        request = Request()
        request.type.which = Request_start_imu_request_tag
        request.type.start_imu_request = StartImuRequest()
        request.type.start_imu_request.timestamp = Timestamp()
        request.type.start_imu_request.timestamp.seconds = timestamp_seconds
        request.type.start_imu_request.timestamp.ms = timestamp_ms
        request.type.start_imu_request.acc_fsr = acc_fsr
        request.type.start_imu_request.gyr_fsr = gyr_fsr
        request.type.start_imu_request.datarate = datarate

        self.send_request(request)

        # Clear the queue before receiving
        with self.start_imu_response_queue.mutex:
            self.start_imu_response_queue.queue.clear()

        while self.start_imu_response_queue.empty():
            self.receive_response()

        return self.start_imu_response_queue.get()

    def stop_imu(self):

        request = Request()
        request.type.which = Request_stop_imu_request_tag
        request.type.stop_imu_request = StopImuRequest()

        self.send_request(request)

    # Send a request to the badge to light an led to identify its self.
    #   If duration_seconds == 0, badge will turn off LED if currently lit.
    # Returns True if request was successfuly sent.
    def identify(self, duration_seconds=10):

        request = Request()
        request.type.which = Request_identify_request_tag
        request.type.identify_request = IdentifyRequest()
        request.type.identify_request.timeout = duration_seconds

        self.send_request(request)

        return True

    def restart(self):

        request = Request()
        request.type.which = Request_restart_request_tag
        request.type.restart_request = RestartRequest()

        self.send_request(request)

        return True

    def get_free_sdc_space(self):

        request = Request()
        request.type.which = Request_free_sdc_space_request_tag
        request.type.free_sdc_space_request = FreeSDCSpaceRequest()

        self.send_request(request)

        # Clear the queue before receiving
        with self.free_sdc_space_response_queue.mutex:
            self.free_sdc_space_response_queue.queue.clear()

        while self.free_sdc_space_response_queue.empty():
            self.receive_response()

        return self.free_sdc_space_response_queue.get()

