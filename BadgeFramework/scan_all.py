from bluepy.btle import Scanner, DefaultDelegate

class ScanDelegate(DefaultDelegate):
    def __init__(self):
        DefaultDelegate.__init__(self)

    # def handleDiscovery(self, dev, isNewDev, isNewData):
    #     if isNewDev:
    #         print "Discovered device", dev.addr
    #     elif isNewData:
    #         print "Received new data from", dev.addr

scanner = Scanner().withDelegate(ScanDelegate())
devices = scanner.scan(5.0)
device_temp_name = 'HDBDG'
midges = []

for dev in devices:
    for (adtype, desc, value) in dev.getScanData():
        if desc == 'Complete Local Name' and value == device_temp_name:
            midges.append(dev)

for midge in midges:
    print ("Device %s, RSSI=%d dB" % (midge.addr, midge.rssi))