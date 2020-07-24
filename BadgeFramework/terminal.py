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

# Enable debug output.
#logging.basicConfig(stream=sys.stdout, level=logging.DEBUG)
# Main Loop of Badge Terminal

def main():
	num_args = len(sys.argv) # Get the arguments list
	if num_args != 2:
		print("Please enter badge MAC address")
		return

	device_addr = sys.argv[1]
	print("Connecting to badge", device_addr)
	connection = BLEBadgeConnection.get_connection_to_badge(device_addr)

	if not connection:
		print("Could not find nearby OpenBadge. :( Please try again!")
		return

	connection.connect()
	badge = OpenBadge(connection)

	print("Connected!")

	def print_help(args):
		print("Available commands: [optional arguments]")
		print("  status [new badge id] [group number] (id + group must be set together)")
		print("  start_microphone")
		print("  stop_microphone")
		print("  start_scan")
		print("  stop_scan")
		print("  start_imu")
		print("  stop_imu")
		print("  identify [led duration seconds | 'off']")
		print("  restart")
		print("  get_free_space")
		print("  help")
		print("All commands use current system time as transmitted time.")
		print("Default arguments used where not specified.")

	def handle_status_request(args):
		if len(args) == 1:
			print(badge.get_status())
		elif len(args) == 2:
			print("Badge ID and Group Number Must Be Set Simultaneously")
		elif len(args) == 3:
			new_id = int(args[1])
			group_number = int(args[2])
			print(badge.get_status(new_id=new_id, new_group_number=group_number))
		else:
			print("Invalid Syntax: status [new badge id] [group number]")

	def handle_start_microphone_request(args):
		print(badge.start_microphone())

	def handle_stop_microphone_request(args):
		badge.stop_microphone()


	def handle_start_scan_request(args):
		print(badge.start_scan())

	def handle_stop_scan_request(args):
		badge.stop_scan()


	def handle_start_imu_request(args):
		print(badge.start_imu())

	def handle_stop_imu_request(args):
		badge.stop_imu()


	def handle_identify_request(args):
		if len(args) == 1:
			badge.identify()
		elif len(args) == 2:
			if args[1] == "off":
				badge.identify(duration_seconds=0)
			else:
				badge.identify(duration_seconds=int(args[1]))
		else:
			print("Invalid Syntax: identify [led duration seconds | 'off']")
			return

	def handle_restart_request(args):
		print(badge.restart())

	def handle_get_free_space(args):
		print(badge.get_free_sdc_space())


	command_handlers = {
		"help": print_help,
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

	while True:
		sys.stdout.write("> ")
		# [:-1] removes newline character
		command = sys.stdin.readline()[:-1]
		if command == "exit":
			connection.disconnect()
			break

		command_args = command.split(" ")
		if command_args[0] in command_handlers:
			command_handlers[command_args[0]](command_args)
		else:
			print("Command Not Found!")
			print_help({})

if __name__ == "__main__":
	main()
