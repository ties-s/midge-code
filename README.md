# "Midge", an SPCL badge 

![The MINGLE MIDGE](https://github.com/TUDelft-SPC-Lab/spcl_midge_hardware/raw/master/v2.3.jpg)

## Hardware
Hardware design files are in a [separate hardware repo](https://github.com/TUDelft-SPC-Lab/spcl_midge_hardware).

## Advertisment Packet structure
The advertisment packet has a field called "manufacturer specific data", with type 0xFF. It should start at the 12th byte. Its length is 11 bytes:

typedef struct
{
    uint8_t battery;
    uint8_t status_flags;
    uint16_t ID;
    uint8_t group;
    uint8_t MAC[6];
} custom_advdata_t;

MAC is set during init, ID and group with the status command.
Battery is in percentage, and the status flags are :
bit 0: clock sync
bit 1: microphone 
bit 2: scanner
bit 3: imu

0 is not active, 1 active.

## Data format

### Audio
On "high" audio is stereo, 20kHz, 16bit per channel PCM.
On "low" it is subsampled by a factor of 32, to 625Hz.

It is only timestamped when the file is created (filename is seconds).

### IMU
Filename is again timestamped, but also each sample (32 bytes):

accelerometer, gyro, magnetometer sample example:
2dd4 a69d 016d 0000 0000 3b40 0000 bc48 2000 3f83 0000 000c 0000 ffce 0000 1064

First 8 bytes are the timestamp:
2dd4 a69d 016d 0000   = 0000016da69d2dd4 = 1570458381780 milliseconds = 07/10/2019 2:26:780

4 bytes float per axis:
0000 3b40   0000 bc48   2000 3f83

padding for data alignment
0000 000c 0000 ffce 0000 1064

Rotation vector:
Timestamp is the same 8 bytes, then 4 floats per quaternion, 4 fewer bytes for padding.

### Scanner
16 byte length
Same 8 byte timestamp.
2 bytes ID
1 int8_t rssi (signed)
1 byte group
4 bytes padding


**the rest should be as you've asked on the requirements, please let me know if something is not clear**

--- 



## BadgeFramework Hub
Runs on Ubuntu (21.10) on RPI platform.
Raspberry pi 3B+ and 4 work.

### Installation instructions
``` sh
sudo apt install python3 python3-pip libglib2.0-dev pip3 install bluepy pandas
```

### Start instructions
1. clone this repository
2. change to the BadgeFramework directory inside the cloned repo folder:
	``` sh
	cd BadgeFramework
	```
3. Modify the `sample_mapping_file.csv` to include your sensors. See the maping dropdown. 
    <details><summary>Mapings between device number and mac address</summary>


    ``` csv
    Participant Id,Mac Address
    1,d5:46:26:05:1f:2f
    2,c3:4c:d5:ba:b1:44
    3,e5:93:96:ca:ee:c4
    4,f9:39:24:a9:04:f1
    5,d2:fd:13:bd:81:39
    6,de:94:80:39:25:be
    7,fb:f5:d5:84:a1:68
    8,e2:6e:4e:21:f1:a4
    9,f5:40:84:e2:9a:16
    10,e8:44:59:c0:39:de
    11,d7:11:de:c6:c8:e3
    12,fa:24:bd:55:c7:ab
    13,d6:12:78:32:80:19
    14,e8:03:31:16:ce:3a
    15,d8:ae:5b:aa:55:ae
    16,f5:ef:4c:9b:55:de
    17,c3:2b:78:b5:c2:4a
    18,c4:3e:0b:bc:e6:92
    19,d9:0d:2e:b7:cc:6b
    20,e6:cc:57:a3:6b:57
    21,c6:30:71:35:02:5a
    22,fb:42:af:eb:ba:3c
    23,d2:91:1c:b9:6f:5c
    24,c1:a4:56:be:7f:7e
    25,f2:26:6c:f5:ca:e5
    26,cb:14:eb:9a:5c:a3
    27,db:03:30:a6:ee:86
    28,fc:3f:62:28:17:b4
    29,ce:22:14:de:40:38
    30,e3:5d:06:9a:55:0f
    31,d4:56:f4:e1:ef:f1
    32,d2:4f:7c:01:93:0e
    33,da:03:10:bb:61:f5
    34,f5:e7:4d:77:4e:74
    35,fd:f3:eb:4b:d0:8c
    36,dc:bc:ed:19:1f:09
    37,c5:61:a9:e9:83:ef
    39,e7:03:db:e9:16:3b
    40,fe:82:88:fa:36:62
    41,dc:a4:f4:7e:45:fb
    42,f6:01:ad:d1:18:23
    43,e0:c3:d6:2e:ab:44
    44,de:7f:7f:a2:9c:f5
    45,fe:71:db:20:4f:34
    46,dc:11:e6:20:81:d8
    47,d9:98:8a:68:f0:89
    48,f7:5a:78:fd:21:46
    49,e9:59:12:26:a7:63
    50,d0:c5:cc:ef:90:e2
    ```


    </details>
4. Start the hub
``` sh
python3 hub_V1.py

```
5. enter the `start` command

### usage instructions 
1. Turn on Midges,make sure all of them blinked orange couple times. This means that they’re turned on and now on stand-by
3. run hub_V1.py
4. type “start” to start data collection on all midges
5. all midges should have a solid orange light when they’re recording

### During data collection:
1. say verbally “experiment # start”
2. play the audiovisual events
3. say verbally “experiment # end”
4. In case of multiple takes, might need to stop recording on midges and start again.



