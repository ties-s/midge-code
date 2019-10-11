import matplotlib.pyplot as plt
import struct
from datetime import datetime as dt
import numpy as np
import pandas as pd
import seaborn as sns

class IMUParser(object):

    def __init__(self,filename):
        self.filename = filename
        self.path_accel = filename+'_accel'
        self.path_gyro = filename+'_gyr'
        self.path_mag = filename+'_mag'
        self.path_rotation = filename+'_rotation'

    def parse_generic(self,sensorname):
        data = []
        timestamps = []
        with open(sensorname, "rb") as f:
            byte = f.read()
            i=0
            while True:
                ts_bytes = byte[0+i:8+i]
                data_bytes = byte[8+i:20+i]
                if (len(data_bytes)) == 12 and (len(ts_bytes) == 8):
                    ts = struct.unpack('<Q',ts_bytes)
                    x,y,z = struct.unpack('<fff', data_bytes)
                    data.append([x,y,z])
                    timestamps.append(ts)
                    i = i + 32
                else:
                    break
        data_xyz = np.asarray(data)
        timestamps = np.asarray(timestamps)
        timestamps_dt = [dt.fromtimestamp(float(x)/1000) for x in timestamps]
        df = pd.DataFrame(timestamps_dt, columns=['time'])
        df['X'] = data_xyz[:,0]
        df['Y'] = data_xyz[:,1]
        df['Z'] = data_xyz[:,2]
        return df

    def parse_accel(self):
        self.accel_df = self.parse_generic(self.path_accel)

    def parse_gyro(self):
        self.gyro_df = self.parse_generic(self.path_gyro)

    def parse_mag(self):
        self.mag_df = self.parse_generic(self.path_mag)

    def parse_rot(self):
        rotation = []
        timestamps = []
        with open(self.path_rotation, "rb") as f:
            byte = f.read()
            i=0
            while True:
                ts_bytes = byte[0+i:8+i]
                rot_bytes = byte[8+i:24+i]
                if (len(rot_bytes)) == 16 and (len(ts_bytes) == 8):
                    ts = struct.unpack('<Q',ts_bytes)
                    q1,q2,q3,q4 = struct.unpack('<ffff', rot_bytes)
                    rotation.append([q1,q2,q3,q4])
                    timestamps.append(ts)
                    i = i + 32
                else:
                    break
        rotation_xyz = np.asarray(rotation)
        timestamps = np.asarray(timestamps)
        timestamps_dt = [dt.fromtimestamp(float(x)/1000) for x in timestamps]
        df = pd.DataFrame(timestamps_dt, columns=['time'])
        df['a'] = rotation_xyz[:,0]
        df['b'] = rotation_xyz[:,1]
        df['c'] = rotation_xyz[:,2]
        df['d'] = rotation_xyz[:,2]
        self.rot_df = df

    def plot_and_save(self,df,sensor):
        fname = self.filename + '_' + sensor +'.png'
        ax = df.plot(x='date')
        fig = ax.get_figure()
        fig.savefig(fname)

    def save_dataframe(self,a,g,m,r):
        if a:
            self.accel_df.to_pickle(self.path_accel + '.pkl')
            self.accel_df.to_csv










