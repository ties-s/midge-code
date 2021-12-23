import struct
from datetime import datetime as dt
import numpy as np
import pandas as pd
import sys


class IMUParser(object):
    def __init__(self, filename):
        self.filename = filename
        self.path_accel = filename + "_accel"
        self.path_gyro = filename + "_gyr"
        self.path_mag = filename + "_mag"
        self.path_rotation = filename + "_rotation"
        self.path_scan = filename + "_proximity"

    def parse_generic(self, sensorname):
        data = []
        timestamps = []
        with open(sensorname, "rb") as f:
            byte = f.read()
            i = 0
            while True:
                ts_bytes = byte[0 + i : 8 + i]
                data_bytes = byte[8 + i : 20 + i]
                if (len(data_bytes)) == 12 and (len(ts_bytes) == 8):
                    ts = struct.unpack("<Q", ts_bytes)
                    x, y, z = struct.unpack("<fff", data_bytes)
                    if float(ts[0]) < 1603213294000:
                        data.append([x, y, z])
                        timestamps.append(ts[0])
                    sys.stdout.flush()
                    i = i + 32
                else:
                    break
        data_xyz = np.asarray(data)
        timestamps = np.asarray(timestamps)
        timestamps_dt = [dt.fromtimestamp(float(x) / 1000) for x in timestamps]
        df = pd.DataFrame(timestamps_dt, columns=["time"])
        df["X"] = data_xyz[:, 0]
        df["Y"] = data_xyz[:, 1]
        df["Z"] = data_xyz[:, 2]
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
                ts_bytes = byte[0 + i : 8 + i]
                rot_bytes = byte[8 + i : 24 + i]
                if (len(rot_bytes)) == 16 and (len(ts_bytes) == 8):
                    ts = struct.unpack("<Q", ts_bytes)
                    q1, q2, q3, q4 = struct.unpack("<ffff", rot_bytes)
                    if float(ts[0]) < 1603213294000:
                        rotation.append([q1, q2, q3, q4])
                        timestamps.append(ts[0])
                        # print (ts[0])
                    i = i + 32
                else:
                    break
        rotation_xyz = np.asarray(rotation)
        timestamps = np.asarray(timestamps)
        timestamps_dt = [dt.fromtimestamp(float(x) / 1000) for x in timestamps]
        df = pd.DataFrame(timestamps_dt, columns=["time"])
        df["a"] = rotation_xyz[:, 0]
        df["b"] = rotation_xyz[:, 1]
        df["c"] = rotation_xyz[:, 2]
        df["d"] = rotation_xyz[:, 2]
        self.rot_df = df

    def parse_scanner(self):
        ids = []
        rssis = []
        timestamps = []
        with open(self.path_scan, "rb") as f:
            byte = f.read()
            i = 0
            while True:
                ts_bytes = byte[0 + i : 8 + i]
                id_bytes = byte[8 + i : 10 + i]
                group_byte = byte[10 + i : 11 + i]
                rssi_byte = byte[11 + i : 12 + i]
                if (
                    (len(id_bytes) == 2)
                    and (len(ts_bytes) == 8)
                    and (len(rssi_byte) == 1)
                    and (len(group_byte) == 1)
                ):
                    ts = struct.unpack("<Q", ts_bytes)
                    scanned_id = struct.unpack("<H", id_bytes)
                    # Group_id is read in the next line, unused
                    _ = struct.unpack("<B", group_byte)
                    scanned_rssi = struct.unpack("<b", rssi_byte)
                    ids.append(scanned_id)
                    rssis.append(scanned_rssi)
                    timestamps.append(ts)
                    i = i + 16
                else:
                    break
        ids = np.asarray(ids)
        rssis = np.asarray(rssis)
        timestamps = np.asarray(timestamps)
        timestamps_dt = [dt.fromtimestamp(float(x) / 1000) for x in timestamps]
        df = pd.DataFrame(timestamps_dt, columns=["time"])
        df["id"] = ids
        df["rssi"] = rssis
        self.scan_df = df

    def plot_and_save(self, a, g, m):
        if a:
            fname = self.filename + "_accel.png"
            ax = self.accel_df.plot(x="time")
            fig = ax.get_figure()
            fig.savefig(fname)
        if g:
            fname = self.filename + "_gyro.png"
            ax = self.gyro_df.plot(x="time")
            fig = ax.get_figure()
            fig.savefig(fname)
        if m:
            fname = self.filename + "_mag.png"
            ax = self.mag_df.plot(x="time")
            fig = ax.get_figure()
            fig.savefig(fname)

    def save_dataframes(self, a, g, m, r, s):
        if a:
            self.accel_df.to_pickle(self.path_accel + ".pkl")
            self.accel_df.to_csv(self.path_accel + ".csv")
        if g:
            self.gyro_df.to_pickle(self.path_gyro + ".pkl")
            self.gyro_df.to_csv(self.path_gyro + ".csv")
        if m:
            self.mag_df.to_pickle(self.path_mag + ".pkl")
            self.mag_df.to_csv(self.path_mag + ".csv")
        if r:
            self.rot_df.to_pickle(self.path_rotation + ".pkl")
            self.rot_df.to_csv(self.path_rotation + ".csv")
        if s:
            self.scan_df.to_pickle(self.path_scan + ".pkl")
            self.scan_df.to_csv(self.path_scan + ".csv")


def str2bool(v):
    if isinstance(v, bool):
        return v
    if v.lower() in ("yes", "true", "t", "y", "1"):
        return True
    elif v.lower() in ("no", "false", "f", "n", "0"):
        return False
    else:
        raise argparse.ArgumentTypeError("Boolean value expected.")


def main(fn, acc, mag, gyr, rot, plot, scn):
    parser = IMUParser(fn)
    if acc:
        parser.parse_accel()
    if mag:
        parser.parse_mag()
    if gyr:
        parser.parse_gyro()
    if rot:
        parser.parse_rot()
    if scn:
        parser.parse_scanner()
    parser.save_dataframes(acc, mag, gyr, rot, scn)
    if plot:
        parser.plot_and_save(acc, mag, gyr)


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(
        description="Parser for the IMU and scanner data obtained from Minge Midges\
    (Acceleration, Gyroscope, Magnetometer, Rotation, Scanner)"
    )
    parser.add_argument("--fn", required=True, help="Please enter the path to the file")
    parser.add_argument(
        "--acc",
        default=True,
        type=str2bool,
        help="Check to parse and save acceleration data",
    )
    parser.add_argument(
        "--mag",
        default=True,
        type=str2bool,
        help="Check to parse and save magnetometer data",
    )
    parser.add_argument(
        "--gyr",
        default=True,
        type=str2bool,
        help="Check to parse and save gyroscope data",
    )
    parser.add_argument(
        "--rot",
        default=True,
        type=str2bool,
        help="Check to parse and save rotation data",
    )
    parser.add_argument(
        "--scn", default=True, type=str2bool, help="Check to parse and save scan data"
    )
    parser.add_argument(
        "--plot", default=True, type=str2bool, help="Check to plot the parsed data"
    )
    args = parser.parse_args()
    main(
        fn=args.fn,
        acc=args.acc,
        mag=args.mag,
        gyr=args.gyr,
        rot=args.rot,
        scn=args.scn,
        plot=args.plot,
    )
