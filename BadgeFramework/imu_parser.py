import struct
from datetime import datetime as dt
import numpy as np
import pandas as pd
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
'''
A timestamp in milliseconds used to verify that the timestamp is plausible

'''
MAX_TIMESTAMP: Final = 1767139200000
MAX_TIMESTAMP_HUMAN_READABLE: Final = dt.fromtimestamp(MAX_TIMESTAMP/1000)
print(f"Max timestamp for data is {MAX_TIMESTAMP_HUMAN_READABLE} local time")


def get_script_directory() -> str:
    sourceFile = getsourcefile(lambda: 0)
    if sourceFile == None:
        sys.exit(
            f"Stopping: could not get sourcefile")
    return os.path.dirname(abspath(sourceFile))


def create_path_if_not_exists(path: str, force: bool):
    shortpath = os.sep.join(os.path.normpath(path).split(os.sep)[-2:])
    if os.path.exists(path) and len(os.listdir(path)) != 0 and force:
        print(f"removing path {shortpath}")
        shutil.rmtree(path)
    if not os.path.exists(path):
        print(f"creating  non existing directory: {path}")
        os.makedirs(path)
        return
    if len(os.listdir(path)) != 0 and not force:
        sys.exit(
            f"Stopping: directory {shortpath} is not empty, retry with -f if you want to override the data")


def fullTimeName(timestamp: int):
    date = datetime.fromtimestamp(timestamp).strftime('%Y-%m-%d_%H:%M:%S')
    return f"{timestamp}_{date}"


class SDFilesCopier(object):

    def __init__(self, input_directory: str, timestamp: int,  force: bool, create_path: bool = True):
        self.force = force
        self.full_file_name = fullTimeName(timestamp)
        self.create_path = create_path
        self.raw_output_directory = os.path.join(get_script_directory(
        ), OUTPUT_RAW_DIRECTORY, self.full_file_name)
        full_path = os.path.join(input_directory, str(timestamp))
        self.path_accel_sd = full_path + ACCELERATION_POSTFIX
        self.path_gyro_sd = full_path + GYROSCOPE_POSTFIX
        self.path_mag_sd = full_path + MAGNETOMETER_POSTFIX
        self.path_rotation_sd = full_path + ROTATION_POSTFIX

    def outputfile(self, postfix: str):
        return os.path.join(self.raw_output_directory, f"{self.full_file_name}{postfix}")

    def moveFiles(self):
        print(f"moving raw files to {self.raw_output_directory}")
        if self.create_path:
            create_path_if_not_exists(
                self.raw_output_directory, force=self.force)

        shutil.copy2(self.path_accel_sd, self.outputfile(ACCELERATION_POSTFIX))
        shutil.copy2(self.path_gyro_sd, self.outputfile(GYROSCOPE_POSTFIX))
        shutil.copy2(self.path_mag_sd, self.outputfile(MAGNETOMETER_POSTFIX))
        shutil.copy2(self.path_rotation_sd, self.outputfile(ROTATION_POSTFIX))

    def parserFromCopiedFiles(self):
        return IMUParser(input_directory=self.raw_output_directory, full_file_name=self.full_file_name, force=self.force, create_path=self.create_path)


class IMUParser(object):

    def __init__(self, input_directory: str, full_file_name: str, force: bool, create_path: bool):
        self.force = force
        self.create_path = create_path
        self.file_base_path = input_directory
        self.raw_output_directory = os.path.join(get_script_directory(
        ), OUTPUT_RAW_DIRECTORY, full_file_name)
        full_path = os.path.join(input_directory, str(full_file_name))

        self.path_accel = full_path+ACCELERATION_POSTFIX
        self.path_gyro = full_path+GYROSCOPE_POSTFIX
        self.path_mag = full_path+MAGNETOMETER_POSTFIX
        self.path_rotation = full_path+ROTATION_POSTFIX
        self.check_if_file_exists(self.path_accel)
        self.check_if_file_exists(self.path_gyro)
        self.check_if_file_exists(self.path_mag)
        self.check_if_file_exists(self.path_rotation)

        # REMOVE THIS
        self.file_base_name = os.path.basename(self.file_base_path)
        self.output_directory_path = os.path.join(get_script_directory(),
                                                  OUTPUT_DIRECTORY, self.file_base_name)

    def check_if_file_exists(self, path: str):
        if not os.path.exists(path):
            sys.exit(f"Stopping: the following file does not exist: {path}")

    def parse_generic(self, sensorname: str):
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
                    if float(ts[0]) > MAX_TIMESTAMP:
                        sys.exit(
                            f"""Stopping: The timestamp in the file exeeds the max timestamp: {MAX_TIMESTAMP_HUMAN_READABLE}.\n
                            If the data is recorded after the max timestamp, increase it, otherwise check the data format""")

                    data.append([x, y, z])
                    timestamps.append(ts[0])
                    sys.stdout.flush()
                    i = i + 24
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
                    if float(ts[0]) > MAX_TIMESTAMP:
                        sys.exit(
                            f"""Stopping: The timestamp in the file exeeds the max timestamp: {MAX_TIMESTAMP_HUMAN_READABLE}.\n 
                            If the data is recorded after the max timestamp, increase it, otherwise check the data format""")

                    rotation.append([q1, q2, q3, q4])
                    timestamps.append(ts[0])
                    # print (ts[0])
                    i = i + 24
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

    def plot_and_save(self, acc: bool, gyr: bool, mag: bool) -> None:
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

    def save_dataframes(self, acc: bool, gyr: bool, mag: bool, rot: bool) -> None:
        if self.create_path:
            create_path_if_not_exists(
                self.output_directory_path, force=self.force)
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


def processDirectory(directory: str) -> list[int]:
    if not os.path.isdir(directory):
        sys.exit(
            f"Stopping: The given path is not a directory")
    shortpath: str = os.sep.join(
        os.path.normpath(directory).split(os.sep)[-2:])

    print(f"using files from directory: {shortpath}")
    filenames_in_directory = [f for f in os.listdir(
        directory) if os.path.isfile(os.path.join(directory, f))]

    non_empty_files_in_directory: list[str] = []
    for file in filenames_in_directory:
        if os.path.getsize(os.path.join(directory, file)) <= 0:
            print(f"File {file} is empty, not analysing this file")
            continue
        non_empty_files_in_directory.append(file)

    if len(non_empty_files_in_directory) == 0:
        sys.exit(
            f"Stopping: no non empty files where found in directory: {shortpath}")

    unique_timestamps_of_files: list[int] = []
    for file in non_empty_files_in_directory:
        possible_timestamp_part: str = file.split("_")[0]
        try:
            timestamp = int(possible_timestamp_part)
            if not timestamp in unique_timestamps_of_files:
                print(
                    f"found file with the following timestamp: {timestamp} (these files will be used if enabled: {timestamp}{ACCELERATION_POSTFIX}, {timestamp}{GYROSCOPE_POSTFIX}, {timestamp}{MAGNETOMETER_POSTFIX} and {timestamp}{ROTATION_POSTFIX})")
                unique_timestamps_of_files.append(timestamp)
        except:
            continue
    return unique_timestamps_of_files


def main(path_directory_sd: str, acc: bool, mag: bool, gyr: bool, rot: bool, plot: bool, force: bool):
    unique_timestamps_of_files = processDirectory(path_directory_sd)
    create_path = True
    for timestamp in unique_timestamps_of_files:
        sdfiles = SDFilesCopier(
            input_directory=path_directory_sd, timestamp=timestamp, force=force, create_path=create_path)
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
        # after first run, the directories should not be deleted and recreated
        create_path = False


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description='Parser and copier for the IMU data obtained from Minge Midges\
    (Acceleration, Gyroscope, Magnetometer, Rotation)')
    parser.add_argument('directory',
                        help='Please enter the path to the directory to read from')
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

    main(path_directory_sd=args.directory, acc=args.no_acc, mag=args.no_mag,
         gyr=args.no_gyr, rot=args.no_rot, plot=args.no_plot, force=args.f)
