import pandas as pd
import sys
from hub_utilities_V1 import (
    start_recording_all_devices,
    stop_recording_all_devices,
    timeout_input,
    choose_function,
    synchronise_and_check_all_devices,
    get_logger
)
from hub_connection_V1 import Connection

if __name__ == "__main__":
    df = pd.read_csv("mappings_all.csv")
    logger = get_logger("hub_main")
    while True:
        logger.info("Type start to start data collection or stop to finish data "
                    + "collection.")
        sys.stdout.write("> ")
        sys.stdout.flush()
        command = sys.stdin.readline()[:-1]
        if command == "start":
            logger.info("Connecting to the midges for starting the recordings.")
            start_recording_all_devices(df)
            logger.info("Loop for starting the devices is finished.")
            while True:
                ti = timeout_input(poll_period=0.05)
                s = ti.input(
                    prompt="Type int if you would like to enter interactive shell.\n"
                    + ">",
                    timeout=10.0,
                    extend_timeout_with_input=False,
                    require_enter_to_confirm=True,
                )
                if s == "int":
                    logger.info(
                        "Welcome to the interactive shell. Please type the id of the"
                        + " midge you want to connect."
                    )
                    logger.info(
                        "Type exit if you would like to stop recording for all devices."
                    )
                    sys.stdout.write("> ")
                    sys.stdout.flush()
                    command = sys.stdin.readline()[:-1]
                    if command == "exit":
                        logger.info("Stopping the recording of all devices.")
                        sys.stdout.flush()
                        stop_recording_all_devices(df)
                        logger.info("Devices are stopped.")
                        sys.stdout.flush()
                        break
                    command_args = command.split(" ")
                    try:
                        current_mac_addr = (
                            df.loc[df["Participant Id"] == int(command)]["Mac Address"]
                        ).values[0]
                    except Exception:
                        logger.info('Mac address for the midge ' + str(command)
                                    + ' is not found.')
                        continue
                    try:
                        cur_connection = Connection(int(command), current_mac_addr)
                    except Exception as error:
                        logger.info("While connecting to midge " + str(command)
                                    + ", following error occurred:" + str(error))
                        sys.stdout.flush()
                        continue
                    logger.info("Connected to the midge " + str(command) + "."
                                + " For available commands, please type help.")
                    sys.stdout.flush()
                    while True:
                        sys.stdout.write("> ")
                        command = sys.stdin.readline()[:-1]
                        command_args = command.split(" ")
                        if command == "exit":
                            cur_connection.disconnect()
                            logger.info("Disconnected from the midge.")
                            break
                        try:
                            out = choose_function(cur_connection, command_args[0])
                            if out is not None:
                                logger.info("Midge returned following"
                                            + " status: " + str(out))
                                sys.stdout.flush()
                        except Exception as error:
                            logger.info(str(error))
                            sys.stdout.flush()
                            cur_connection.print_help()
                            continue
                else:
                    logger.info("Synchronisation is starting. Please wait till it ends")
                    synchronise_and_check_all_devices(df)
                    logger.info("Synchronisation is finished.")
                    sys.stdout.flush()
        elif command == "stop":
            logger.info("Stopping data collection.")
            sys.stdout.flush()
            quit(0)
        else:
            logger.info(
                "Command not found, please type start or stop to start or stop data "
                + "collection."
            )
            sys.stdout.flush()
