import sys

import pandas as pd

from hub_utilities_V1 import *

if __name__ == "__main__":
    df = pd.read_csv('sample_mapping_file.csv')
    while True:
        print("Type start to start data collection or stop to finish data collection.")
        sys.stdout.write("> ")
        sys.stdout.flush()
        command = sys.stdin.readline()[:-1]
        if command == "start":
            start_recording_all_devices(df)
            while True:
                ti = timeout_input(poll_period=0.05)
                s = ti.input(prompt='Type int if you would like to enter interactive shell.\n'+'>', timeout=10.0,
                            extend_timeout_with_input=False, require_enter_to_confirm=True)
                if s == "int":
                    print("Welcome to the interactive shell. Please type the id of the Midge you want to connect.")
                    print("Type exit if you would like to stop recording for all devices.")
                    sys.stdout.write("> ")
                    sys.stdout.flush()
                    command = sys.stdin.readline()[:-1]
                    if command == "exit":
                        print("Stopping the recording of all devices.")
                        sys.stdout.flush()
                        stop_recording_all_devices(df)
                        print("Devices are stopped.")
                        sys.stdout.flush()
                        break
                    command_args = command.split(" ")
                    current_mac_addr= (df.loc[df['Participant Id'] == int(command)]['Mac Address']).values[0]
                    try:
                        cur_connection = Connection(int(command),current_mac_addr)
                    except Exception as error:
                        print (str(error))
                        sys.stdout.flush()
                        continue
                    print ("Connected to the badge. For available commands, please type help.")
                    sys.stdout.flush()
                    while True:
                        sys.stdout.write("> ")
                        command = sys.stdin.readline()[:-1]
                        command_args = command.split(" ")
                        if command == "exit":
                            cur_connection.disconnect()
                            break
                        try:
                            out = choose_function(cur_connection,command_args[0])
                            if out != None:
                                print (out)
                                sys.stdout.flush()
                        except Exception as error:
                            print (str(error))
                            print(" Command not found!")
                            sys.stdout.flush()
                            cur_connection.print_help()
                            continue
                else:
                    print('Synchronisation is starting. Please wait till it ends.')
                    synchronise_and_check_all_devices(df)
                    print('Synchronisation is finished.')
                    sys.stdout.flush()
        elif command == "stop":
            print("Stopping data collection.")
            sys.stdout.flush()
            quit(0)
        else:
            print("Command not found, please type start or stop to start or stop data collection.")
            sys.stdout.flush()

