import logging
import threading
logger = logging.getLogger(__name__)
f_handler = logging.FileHandler('sync.log')
f_handler.setLevel(logging.WARNING)
logger.addHandler(f_handler)

class Synchronizer():
    def __init__(self, connections):
        self.connections = connections
        self.sync_index = 0
        self.thread = None
        
    def add(self, connection):
        self.connections.append(connection)

    def __str__(self):
        pass
    
    def _sync(self, stop):
        while True:
            for conn in self.connections:
                try:
                    status = conn.get_status()
                except Exception as e:
                    logger.warning(str(e))
                if stop():
                    return

    def start(self):
        self.stop_thread = False
        self.thread = threading.Thread(target=self._sync, args=(lambda: self.stop_thread,))
        self.thread.start()

    def stop(self):
        self.stop_thread = True
        self.thread.join()        

    def __del__(self):
        self.stop()



