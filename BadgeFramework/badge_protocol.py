import struct

Request_status_request_tag = 1
Request_start_microphone_request_tag = 2
Request_stop_microphone_request_tag = 3
Request_start_scan_request_tag = 4
Request_stop_scan_request_tag = 5
Request_start_imu_request_tag = 6
Request_stop_imu_request_tag = 7
Request_identify_request_tag = 27
Request_restart_request_tag = 29
Request_free_sdc_space_request_tag = 30

Response_status_response_tag = 1
Response_start_microphone_response_tag = 2
Response_start_scan_response_tag = 3
Response_start_imu_response_tag = 4
Response_free_sdc_space_response_tag = 5

class _Ostream:
	def __init__(self):
		self.buf = b''
	def write(self, data):
		self.buf += data

class _Istream:
	def __init__(self, buf):
		self.buf = buf
	def read(self, l):
		if(l > len(self.buf)):
			raise Exception("Not enough bytes in Istream to read")
		ret = self.buf[0:l]
		self.buf = self.buf[l:]
		return ret

class Timestamp:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.seconds = 0
		self.ms = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_seconds(ostream)
		self.encode_ms(ostream)
		pass

	def encode_seconds(self, ostream):
		ostream.write(struct.pack('<I', self.seconds))

	def encode_ms(self, ostream):
		ostream.write(struct.pack('<H', self.ms))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_seconds(istream)
		self.decode_ms(istream)
		pass

	def decode_seconds(self, istream):
		self.seconds= struct.unpack('<I', istream.read(4))[0]

	def decode_ms(self, istream):
		self.ms= struct.unpack('<H', istream.read(2))[0]


class BadgeAssignement:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.ID = 0
		self.group = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_ID(ostream)
		self.encode_group(ostream)
		pass

	def encode_ID(self, ostream):
		ostream.write(struct.pack('<H', self.ID))

	def encode_group(self, ostream):
		ostream.write(struct.pack('<B', self.group))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_ID(istream)
		self.decode_group(istream)
		pass

	def decode_ID(self, istream):
		self.ID= struct.unpack('<H', istream.read(2))[0]

	def decode_group(self, istream):
		self.group= struct.unpack('<B', istream.read(1))[0]


class ScanDevice:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.ID = 0
		self.rssi = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_ID(ostream)
		self.encode_rssi(ostream)
		pass

	def encode_ID(self, ostream):
		ostream.write(struct.pack('<H', self.ID))

	def encode_rssi(self, ostream):
		ostream.write(struct.pack('<b', self.rssi))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_ID(istream)
		self.decode_rssi(istream)
		pass

	def decode_ID(self, istream):
		self.ID= struct.unpack('<H', istream.read(2))[0]

	def decode_rssi(self, istream):
		self.rssi= struct.unpack('<b', istream.read(1))[0]



class StatusRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		self.has_badge_assignement = 0
		self.badge_assignement = None
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		self.encode_badge_assignement(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)

	def encode_badge_assignement(self, ostream):
		ostream.write(struct.pack('<B', self.has_badge_assignement))
		if self.has_badge_assignement:
			self.badge_assignement.encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		self.decode_badge_assignement(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)

	def decode_badge_assignement(self, istream):
		self.has_badge_assignement= struct.unpack('<B', istream.read(1))[0]
		if self.has_badge_assignement:
			self.badge_assignement = BadgeAssignement()
			self.badge_assignement.decode_internal(istream)


class StartMicrophoneRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		self.mode = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		self.encode_mode(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)

	def encode_mode(self, ostream):
		ostream.write(struct.pack('<B', self.mode))

	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		self.decode_mode(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)

	def decode_mode(self, istream):
		self.mode= struct.unpack('<B', istream.read(1))[0]

class StopMicrophoneRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		pass


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		pass


class StartScanRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		self.window = 0
		self.interval = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		self.encode_window(ostream)
		self.encode_interval(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)

	def encode_window(self, ostream):
		ostream.write(struct.pack('<H', self.window))

	def encode_interval(self, ostream):
		ostream.write(struct.pack('<H', self.interval))

	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		self.decode_window(istream)
		self.decode_interval(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)

	def decode_window(self, istream):
		self.window= struct.unpack('<H', istream.read(2))[0]

	def decode_interval(self, istream):
		self.interval= struct.unpack('<H', istream.read(2))[0]



class StopScanRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		pass


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		pass


class StartImuRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		self.acc_fsr = 0
		self.gyr_fsr = 0
		self.datarate = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		self.encode_acc_fsr(ostream)
		self.encode_gyr_fsr(ostream)
		self.encode_datarate(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)

	def encode_acc_fsr(self, ostream):
		ostream.write(struct.pack('<H', self.acc_fsr))

	def encode_gyr_fsr(self, ostream):
		ostream.write(struct.pack('<H', self.gyr_fsr))

	def encode_datarate(self, ostream):
		ostream.write(struct.pack('<H', self.datarate))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		self.decode_acc_fsr(istream)
		self.decode_gyr_fsr(istream)
		self.decode_datarate(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)

	def decode_acc_fsr(self, istream):
		self.acc_fsr = struct.unpack('<H', istream.read(2))[0]

	def decode_gyr_fsr(self, istream):
		self.gyr_fsr = struct.unpack('<H', istream.read(2))[0]

	def decode_datarate(self, istream):
		self.datarate= struct.unpack('<H', istream.read(2))[0]


class StopImuRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		pass


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		pass



class IdentifyRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timeout = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timeout(ostream)
		pass

	def encode_timeout(self, ostream):
		ostream.write(struct.pack('<H', self.timeout))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timeout(istream)
		pass

	def decode_timeout(self, istream):
		self.timeout= struct.unpack('<H', istream.read(2))[0]



class RestartRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		pass


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		pass


class FreeSDCSpaceRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		pass


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		pass


class Request:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.type = self._type()
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.type.encode_internal(ostream)
		pass


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.type.decode_internal(istream)
		pass

	class _type:

		def __init__(self):
			self.reset()

		def __repr__(self):
			return str(self.__dict__)

		def reset(self):
			self.which = 0
			self.status_request = None
			self.start_microphone_request = None
			self.stop_microphone_request = None
			self.start_scan_request = None
			self.stop_scan_request = None
			self.start_imu_request = None
			self.stop_imu_request = None
			self.identify_request = None
			self.restart_request = None
			self.free_sdc_space_request = None
			pass

		def encode_internal(self, ostream):
			ostream.write(struct.pack('<B', self.which))
			options = {
				1: self.encode_status_request,
				2: self.encode_start_microphone_request,
				3: self.encode_stop_microphone_request,
				4: self.encode_start_scan_request,
				5: self.encode_stop_scan_request,
				6: self.encode_start_imu_request,
				7: self.encode_stop_imu_request,
				27: self.encode_identify_request,
				29: self.encode_restart_request,
				30: self.encode_free_sdc_space_request,
			}
			options[self.which](ostream)
			pass

		def encode_status_request(self, ostream):
			self.status_request.encode_internal(ostream)

		def encode_start_microphone_request(self, ostream):
			self.start_microphone_request.encode_internal(ostream)

		def encode_stop_microphone_request(self, ostream):
			self.stop_microphone_request.encode_internal(ostream)

		def encode_start_scan_request(self, ostream):
			self.start_scan_request.encode_internal(ostream)

		def encode_stop_scan_request(self, ostream):
			self.stop_scan_request.encode_internal(ostream)

		def encode_start_imu_request(self, ostream):
			self.start_imu_request.encode_internal(ostream)

		def encode_stop_imu_request(self, ostream):
			self.stop_imu_request.encode_internal(ostream)

		def encode_identify_request(self, ostream):
			self.identify_request.encode_internal(ostream)

		def encode_restart_request(self, ostream):
			self.restart_request.encode_internal(ostream)

		def encode_free_sdc_space_request(self, ostream):
			self.free_sdc_space_request.encode_internal(ostream)


		def decode_internal(self, istream):
			self.reset()
			self.which= struct.unpack('<B', istream.read(1))[0]
			options = {
				1: self.decode_status_request,
				2: self.decode_start_microphone_request,
				3: self.decode_stop_microphone_request,
				4: self.decode_start_scan_request,
				5: self.decode_stop_scan_request,
				6: self.decode_start_imu_request,
				7: self.decode_stop_imu_request,
				27: self.decode_identify_request,
				29: self.decode_restart_request,
				30: self.decode_free_sdc_space_request,
			}
			options[self.which](istream)
			pass

		def decode_status_request(self, istream):
			self.status_request = StatusRequest()
			self.status_request.decode_internal(istream)

		def decode_start_microphone_request(self, istream):
			self.start_microphone_request = StartMicrophoneRequest()
			self.start_microphone_request.decode_internal(istream)

		def decode_stop_microphone_request(self, istream):
			self.stop_microphone_request = StopMicrophoneRequest()
			self.stop_microphone_request.decode_internal(istream)

		def decode_start_scan_request(self, istream):
			self.start_scan_request = StartScanRequest()
			self.start_scan_request.decode_internal(istream)

		def decode_stop_scan_request(self, istream):
			self.stop_scan_request = StopScanRequest()
			self.stop_scan_request.decode_internal(istream)

		def decode_start_imu_request(self, istream):
			self.start_imu_request = StartImuRequest()
			self.start_imu_request.decode_internal(istream)

		def decode_stop_imu_request(self, istream):
			self.stop_imu_request = StopImuRequest()
			self.stop_imu_request.decode_internal(istream)

		def decode_identify_request(self, istream):
			self.identify_request = IdentifyRequest()
			self.identify_request.decode_internal(istream)

		def decode_restart_request(self, istream):
			self.restart_request = RestartRequest()
			self.restart_request.decode_internal(istream)

		def decode_free_sdc_space_request(self, istream):
			self.free_sdc_space_request = FreeSDCSpaceRequest()
			self.free_sdc_space_request.decode_internal(istream)


class StatusResponse:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.clock_status = 0
		self.microphone_status = 0
		self.scan_status = 0
		self.imu_status = 0
		self.time_delta = 0
		self.timestamp = None
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_clock_status(ostream)
		self.encode_microphone_status(ostream)
		self.encode_scan_status(ostream)
		self.encode_imu_status(ostream)
		self.encode_timestamp(ostream)
		self.encode_time_delta(ostream)
		pass

	def encode_clock_status(self, ostream):
		ostream.write(struct.pack('<B', self.clock_status))

	def encode_microphone_status(self, ostream):
		ostream.write(struct.pack('<B', self.microphone_status))

	def encode_scan_status(self, ostream):
		ostream.write(struct.pack('<B', self.scan_status))

	def encode_imu_status(self, ostream):
		ostream.write(struct.pack('<B', self.imu_status))

	def encode_time_delta(self, ostream):
		ostream.write(struct.pack('<i', self.time_delta))

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)



	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_clock_status(istream)
		self.decode_microphone_status(istream)
		self.decode_scan_status(istream)
		self.decode_imu_status(istream)
		self.decode_time_delta(istream)
		self.decode_timestamp(istream)
		pass

	def decode_clock_status(self, istream):
		self.clock_status= struct.unpack('<B', istream.read(1))[0]

	def decode_microphone_status(self, istream):
		self.microphone_status= struct.unpack('<B', istream.read(1))[0]

	def decode_scan_status(self, istream):
		self.scan_status= struct.unpack('<B', istream.read(1))[0]

	def decode_imu_status(self, istream):
		self.imu_status= struct.unpack('<B', istream.read(1))[0]

	def decode_time_delta(self, istream):
		self.time_delta= struct.unpack('<i', istream.read(4))[0]

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)



class StartMicrophoneResponse:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)


class StartScanResponse:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)


class StartImuResponse:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)

class FreeSDCSpaceResponse:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.total_space = 0
		self.free_space = 0
		self.timestamp = None
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_total_space(ostream)
		self.encode_free_space(ostream)
		self.encode_timestamp(ostream)
		pass

	def encode_total_space(self, ostream):
		ostream.write(struct.pack('<I', self.total_space))

	def encode_free_space(self, ostream):
		ostream.write(struct.pack('<I', self.free_space))

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_total_space(istream)
		self.decode_free_space(istream)
		self.decode_timestamp(istream)
		pass

	def decode_total_space(self, istream):
		self.total_space= struct.unpack('<I', istream.read(4))[0]

	def decode_free_space(self, istream):
		self.free_space= struct.unpack('<I', istream.read(4))[0]

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)



class Response:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.type = self._type()
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.type.encode_internal(ostream)
		pass


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.type.decode_internal(istream)
		pass

	class _type:

		def __init__(self):
			self.reset()

		def __repr__(self):
			return str(self.__dict__)

		def reset(self):
			self.which = 0
			self.status_response = None
			self.start_microphone_response = None
			self.start_scan_response = None
			self.start_imu_response = None
			self.free_sdc_space_response = None
			pass

		def encode_internal(self, ostream):
			ostream.write(struct.pack('<B', self.which))
			options = {
				1: self.encode_status_response,
				2: self.encode_start_microphone_response,
				3: self.encode_start_scan_response,
				4: self.encode_start_imu_response,
				5: self.encode_free_sdc_space_response,
			}
			options[self.which](ostream)
			pass

		def encode_status_response(self, ostream):
			self.status_response.encode_internal(ostream)

		def encode_start_microphone_response(self, ostream):
			self.start_microphone_response.encode_internal(ostream)

		def encode_start_scan_response(self, ostream):
			self.start_scan_response.encode_internal(ostream)

		def encode_start_imu_response(self, ostream):
			self.start_imu_response.encode_internal(ostream)

		def encode_free_sdc_space_response(self, ostream):
			self.free_sdc_space_response.encode_internal(ostream)


		def decode_internal(self, istream):
			self.reset()
			self.which= struct.unpack('<B', istream.read(1))[0]
			options = {
				1: self.decode_status_response,
				2: self.decode_start_microphone_response,
				3: self.decode_start_scan_response,
				4: self.decode_start_imu_response,
				5: self.decode_free_sdc_space_response,
			}
			options[self.which](istream)
			pass

		def decode_status_response(self, istream):
			self.status_response = StatusResponse()
			self.status_response.decode_internal(istream)

		def decode_start_microphone_response(self, istream):
			self.start_microphone_response = StartMicrophoneResponse()
			self.start_microphone_response.decode_internal(istream)

		def decode_start_scan_response(self, istream):
			self.start_scan_response = StartScanResponse()
			self.start_scan_response.decode_internal(istream)

		def decode_start_imu_response(self, istream):
			self.start_imu_response = StartImuResponse()
			self.start_imu_response.decode_internal(istream)

		def decode_free_sdc_space_response(self, istream):
			self.free_sdc_space_response = FreeSDCSpaceResponse()
			self.free_sdc_space_response.decode_internal(istream)
