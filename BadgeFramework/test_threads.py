import sys
import time
import logging
import threading

from hub_connection_V1 import Connection
logging.basicConfig(stream=sys.stdout, level=logging.INFO)
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

part1 = 1
dev1 = 'fb:42:af:eb:ba:3c'

part2 = 2
dev2 = 'dc:bc:ed:19:1f:09'

def dummy_sync(current_participant, current_mac):
    while True:
        time.sleep(2)
        logger.info('connected to participant {:d}'.format(current_participant))
        sys.stdout.flush()
        # raise Exception(
        #     'could not connect to participant {current_participant:d}')

def sync(current_participant, current_mac):
    while True:
        try:
            cur_connection = Connection(current_participant, current_mac)
            logger.info('connected to participant {:d}'.format(current_participant))
        except Exception as error:
            logger.info(str(error) + ", cannot synchronise.")
            sys.stdout.flush()
            continue
        try:
            out = cur_connection.handle_status_request()
            logger.info("Status received for the following midge:"
                        + str(current_participant) + ".")
            sys.stdout.flush()
            cur_connection.disconnect()
        except Exception as error:
            logger.info("Status check for participant " + str(current_participant)
                        + " returned the following error: " + str(error) + ".")


x = threading.Thread(target=sync, args=(part1, dev1), daemon=True)
x.start()
sync(part2, dev2)
