import matplotlib.pyplot as plt
import struct
from datetime import datetime as dt
import numpy as np
import pandas as pd
import seaborn as sns
from inspect import getsourcefile
import os
from os.path import abspath
from typing import Final
from datetime import datetime
import shutil
import sys
# Constants
OUTPUT_DIRECTORY: Final = "output"
OUTPUT_RAW_DIRECTORY: Final = "output_raw"

ACCELERATION_POSTFIX: Final = '_accel'
GYROSCOPE_POSTFIX: Final = "_gyr"
MAGNETOMETER_POSTFIX: Final = "_mag"
ROTATION_POSTFIX: Final = "_rotation"


def get_BadgeFrame_Directory():
    return os.path.dirname(abspath(getsourcefile(lambda: 0)))


def create_path_if_not_exists(path, force):
    if os.path.exists(path) and os.listdir(path) != 0 and force:
        try:
            shutil.rmtree(path)
        except OSError as e:
            print("Error: %s - %s." % (e.filename, e.strerror))
    if not os.path.exists(path):
        print(f"creating  non existing directory: {path}")
        os.makedirs(path)
        return
    if os.listdir(path) != 0 and not force:
        shortpath = os.sep.join(os.path.normpath(path).split(os.sep)[-2:])
        sys.exit(
            f"Stopping: directory {shortpath} is not empty, retry with -f if you want to override the data")


def parsedTimeStamp(timestamp):
    return datetime.fromtimestamp(timestamp).strftime('%Y-%m-%d_%H:%M:%S')


def fullTimeName(timestamp):
    return f"{parsedTimeStamp(timestamp)}_{timestamp}"


class SDFiles(object):

    def __init__(self, path_file_sd, force):
        self.force = force
        timestamp = int(os.path.basename(path_file_sd))
        self.full_file_name = fullTimeName(timestamp)
        self.raw_directory = os.path.join(get_BadgeFrame_Directory(
        ), OUTPUT_RAW_DIRECTORY, self.full_file_name)
        self.path_accel_sd = path_file_sd + ACCELERATION_POSTFIX
        self.path_gyro_sd = path_file_sd + GYROSCOPE_POSTFIX
        self.path_mag_sd = path_file_sd+MAGNETOMETER_POSTFIX
        self.path_rotation_sd = path_file_sd+ROTATION_POSTFIX

    def outputfile(self, postfix):
        return os.path.join(self.raw_directory, f"{self.full_file_name}{postfix}")

    def moveFiles(self):
        print(f"moving raw files to {self.raw_directory}")
        create_path_if_not_exists(self.raw_directory, force=self.force)
        shutil.copy2(self.path_accel_sd, self.outputfile(ACCELERATION_POSTFIX))
        shutil.copy2(self.path_gyro_sd, self.outputfile(GYROSCOPE_POSTFIX))
        shutil.copy2(self.path_mag_sd, self.outputfile(MAGNETOMETER_POSTFIX))
        shutil.copy2(self.path_rotation_sd, self.outputfile(ROTATION_POSTFIX))

    def parserFromCopiedFiles(self):
        return IMUParser(os.path.join(self.raw_directory, self.full_file_name), force=self.force)


class IMUParser(object):

    def __init__(self, filename, force):
        self.force = force
        self.file_base_path = filename
        self.path_accel = filename+ACCELERATION_POSTFIX
        self.path_gyro = filename+GYROSCOPE_POSTFIX
        self.path_mag = filename+MAGNETOMETER_POSTFIX
        self.path_rotation = filename+ROTATION_POSTFIX
        self.file_base_name = os.path.basename(self.file_base_path)
        self.output_directory_path = os.path.join(get_BadgeFrame_Directory(),
                                                  OUTPUT_DIRECTORY, self.file_base_name)

    def parse_generic(self, sensorname):
        data = []
        timestamps = []
        with open(sensorname, "rb") as f:
            byte = f.read()
            i = 0
            while True:
                ts_bytes = byte[0+i:8+i]
                data_bytes = byte[8+i:20+i]
                if (len(data_bytes)) == 12 and (len(ts_bytes) == 8):
                    ts = struct.unpack('<Q', ts_bytes)
                    x, y, z = struct.unpack('<fff', data_bytes)
                    data.append([x, y, z])
                    timestamps.append(ts)
                    i = i + 32
                else:
                    break
        data_xyz = np.asarray(data)
        timestamps = np.asarray(timestamps)
        timestamps_dt = [dt.fromtimestamp(float(x)/1000) for x in timestamps]
        df = pd.DataFrame(timestamps_dt, columns=['time'])
        df['X'] = data_xyz[:, 0]
        df['Y'] = data_xyz[:, 1]
        df['Z'] = data_xyz[:, 2]
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
            i = 0
            while True:
                ts_bytes = byte[0+i:8+i]
                rot_bytes = byte[8+i:24+i]
                if (len(rot_bytes)) == 16 and (len(ts_bytes) == 8):
                    ts = struct.unpack('<Q', ts_bytes)
                    q1, q2, q3, q4 = struct.unpack('<ffff', rot_bytes)
                    rotation.append([q1, q2, q3, q4])
                    timestamps.append(ts)
                    i = i + 32
                else:
                    break
        rotation_xyz = np.asarray(rotation)
        timestamps = np.asarray(timestamps)
        timestamps_dt = [dt.fromtimestamp(float(x)/1000) for x in timestamps]
        df = pd.DataFrame(timestamps_dt, columns=['time'])
        df['a'] = rotation_xyz[:, 0]
        df['b'] = rotation_xyz[:, 1]
        df['c'] = rotation_xyz[:, 2]
        df['d'] = rotation_xyz[:, 2]
        self.rot_df = df

    def plot_and_save(self, acc, gyr, mag):
        if acc:
            acc_file_name = os.path.join(
                self.output_directory_path, f"{self.file_base_name}_{ACCELERATION_POSTFIX}.png")
            ax = self.accel_df.plot(x='time')
            fig = ax.get_figure()
            fig.savefig(acc_file_name)
        if gyr:
            gyr_file_name = os.path.join(
                self.output_directory_path, f"{self.file_base_name}_{GYROSCOPE_POSTFIX}.png")
            ax = self.gyro_df.plot(x='time')
            fig = ax.get_figure()
            fig.savefig(gyr_file_name)
        if mag:
            mag_file_name = os.path.join(
                self.output_directory_path, f"{self.file_base_name}_{MAGNETOMETER_POSTFIX}.png")
            ax = self.mag_df.plot(x='time')
            fig = ax.get_figure()
            fig.savefig(mag_file_name)

    def save_dataframes(self, acc, gyr, mag, rot):
        create_path_if_not_exists(self.output_directory_path, force=self.force)
        print(f"saving parsed data to: {self.output_directory_path}")

        if acc:
            acc_file_path = os.path.join(
                self.output_directory_path, f"{self.file_base_name}{ACCELERATION_POSTFIX}")
            self.accel_df.to_pickle(acc_file_path + '.pkl')
            self.accel_df.to_csv(acc_file_path + '.csv')
        if gyr:
            gyr_file_path = os.path.join(
                self.output_directory_path, f"{self.file_base_name}{GYROSCOPE_POSTFIX}")
            self.gyro_df.to_pickle(gyr_file_path + '.pkl')
            self.gyro_df.to_csv(gyr_file_path + '.csv')
        if mag:
            mag_file_path = os.path.join(
                self.output_directory_path, f"{self.file_base_name}{MAGNETOMETER_POSTFIX}")
            self.mag_df.to_pickle(mag_file_path + '.pkl')
            self.mag_df.to_csv(mag_file_path + '.csv')
        if rot:
            rot_file_path = os.path.join(
                self.output_directory_path, f"{self.file_base_name}{ROTATION_POSTFIX}")
            self.rot_df.to_pickle(rot_file_path + '.pkl')
            self.rot_df.to_csv(rot_file_path + '.csv')


def main(file, acc, mag, gyr, rot, plot, force):
    sdfiles = SDFiles(file, force=force)
    sdfiles.moveFiles()
    parser = sdfiles.parserFromCopiedFiles()
    if acc:
        parser.parse_accel()
    if mag:
        parser.parse_mag()
    if gyr:
        parser.parse_gyro()
    if rot:
        parser.parse_rot()
    parser.save_dataframes(acc, mag, gyr, rot)
    if plot:
        parser.plot_and_save(acc, mag, gyr)


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description='Parser for the IMU data obtained from Minge Midges\
    (Acceleration, Gyroscope, Magnetometer, Rotation)')
    parser.add_argument('file',
                        help='Please enter the path to the file')
    parser.add_argument('--no-acc', action="store_false",
                        help='add this flag to not parse accelerometer data (default=FALSE)')
    parser.add_argument('--no-mag', action="store_false",
                        help='add this flag to not parse magnetometer data (default=FALSE)')
    parser.add_argument('--no-gyr', action="store_false",
                        help='add this flag to not parse gyroscope data (default=FALSE) ')
    parser.add_argument('--no-rot', action="store_false",
                        help='add this flag to not parse rotation data from the DMP (default=FALSE)')
    parser.add_argument('--no-plot', action="store_false",
                        help='add this flag to not plot the data from the accelerometer, gyroscope and magnetometer (default=FALSE)')
    parser.add_argument('-f', action="store_true",
                        help=f"add this flag to override the subdirectories in '{OUTPUT_DIRECTORY}' and '{OUTPUT_RAW_DIRECTORY}' (default=FALSE)")
    args = parser.parse_args()
    main(file=args.file, acc=args.no_acc, mag=args.no_mag,
         gyr=args.no_gyr, rot=args.no_rot, plot=args.no_plot, force=args.f)
