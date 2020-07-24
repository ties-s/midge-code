from concurrent.futures import ThreadPoolExecutor

import logging
logger = logging.getLogger(__name__)
f_handler = logging.FileHandler('sync.log')
f_handler.setLevel(logging.WARNING)
logger.addHandler(f_handler)

class Synchronizer():
    def __init__(self, connections, executor=None):
        self.connections = connections
        self.sync_index = 0
        self.executor = None
        if executor is not None:
            self.executor = executor
        
    def add(self, connection):
        self.connections.append(connection)

    def __str__(self):
        pass
    
    def sync_next(self, future=None):
        if future:
            if future.exception() is not None:
                logger.warning(future.exception())
        future = self.connections[self.sync_index].get_status_future()
        future.add_done_callback(self.sync_next)

        self.sync_index += 1
        if self.sync_index >= len(self.connections):
            self.sync_index = 0

    def start(self):
        if self.executor is None:
            self.executor = ThreadPoolExecutor(max_workers=1)
        for conn in self.connections:
            conn.executor = self.executor
        self.sync_next()

    def stop(self):
        self.executor.shutdown(wait=False)
        for conn in self.connections:
            conn.executor = None
        self.executor = None        

    def __del__(self):
        self.stop()



