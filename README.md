# "Midge", an spcl badge 

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

### Error blinks
1 blink - clock
2 blink - timeout
3 blink - ble
4 blink - storage
5 blink - imu related (calibration for example)
