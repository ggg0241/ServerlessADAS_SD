import serial
import time
import picar_4wd as fc
import sys
import tty
import termios
import asyncio

power_val = 50
key = 'status'
print("If you want to quit.Please press q")
def readchar():
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setraw(sys.stdin.fileno())
        ch = sys.stdin.read(1)
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
    return ch

def readkey(getchar_fn=None):
    getchar = getchar_fn or readchar
    c1 = getchar()
    if ord(c1) != 0x1b:
        return c1
    c2 = getchar()
    if ord(c2) != 0x5b:
        return c1
    c3 = getchar()
    return chr(0x10 + ord(c3) - 65)
        

POINT_PER_PACK = 12
PACKAGE_SIZE = 47
HEADER = 0x54
threshold = 4

CrcTable = [  0x00, 0x4d, 0x9a, 0xd7, 0x79, 0x34, 0xe3,
0xae, 0xf2, 0xbf, 0x68, 0x25, 0x8b, 0xc6, 0x11, 0x5c, 0xa9, 0xe4, 0x33,
0x7e, 0xd0, 0x9d, 0x4a, 0x07, 0x5b, 0x16, 0xc1, 0x8c, 0x22, 0x6f, 0xb8,
0xf5, 0x1f, 0x52, 0x85, 0xc8, 0x66, 0x2b, 0xfc, 0xb1, 0xed, 0xa0, 0x77,
0x3a, 0x94, 0xd9, 0x0e, 0x43, 0xb6, 0xfb, 0x2c, 0x61, 0xcf, 0x82, 0x55,
0x18, 0x44, 0x09, 0xde, 0x93, 0x3d, 0x70, 0xa7, 0xea, 0x3e, 0x73, 0xa4,
0xe9, 0x47, 0x0a, 0xdd, 0x90, 0xcc, 0x81, 0x56, 0x1b, 0xb5, 0xf8, 0x2f,
0x62, 0x97, 0xda, 0x0d, 0x40, 0xee, 0xa3, 0x74, 0x39, 0x65, 0x28, 0xff,
0xb2, 0x1c, 0x51, 0x86, 0xcb, 0x21, 0x6c, 0xbb, 0xf6, 0x58, 0x15, 0xc2,
0x8f, 0xd3, 0x9e, 0x49, 0x04, 0xaa, 0xe7, 0x30, 0x7d, 0x88, 0xc5, 0x12,
0x5f, 0xf1, 0xbc, 0x6b, 0x26, 0x7a, 0x37, 0xe0, 0xad, 0x03, 0x4e, 0x99,
0xd4, 0x7c, 0x31, 0xe6, 0xab, 0x05, 0x48, 0x9f, 0xd2, 0x8e, 0xc3, 0x14,
0x59, 0xf7, 0xba, 0x6d, 0x20, 0xd5, 0x98, 0x4f, 0x02, 0xac, 0xe1, 0x36,
0x7b, 0x27, 0x6a, 0xbd, 0xf0, 0x5e, 0x13, 0xc4, 0x89, 0x63, 0x2e, 0xf9,
0xb4, 0x1a, 0x57, 0x80, 0xcd, 0x91, 0xdc, 0x0b, 0x46, 0xe8, 0xa5, 0x72,
0x3f, 0xca, 0x87, 0x50, 0x1d, 0xb3, 0xfe, 0x29, 0x64, 0x38, 0x75, 0xa2,
0xef, 0x41, 0x0c, 0xdb, 0x96, 0x42, 0x0f, 0xd8, 0x95, 0x3b, 0x76, 0xa1,
0xec, 0xb0, 0xfd, 0x2a, 0x67, 0xc9, 0x84, 0x53, 0x1e, 0xeb, 0xa6, 0x71,
0x3c, 0x92, 0xdf, 0x08, 0x45, 0x19, 0x54, 0x83, 0xce, 0x60, 0x2d, 0xfa,
0xb7, 0x5d, 0x10, 0xc7, 0x8a, 0x24, 0x69, 0xbe, 0xf3, 0xaf, 0xe2, 0x35,
0x78, 0xd6, 0x9b, 0x4c, 0x01, 0xf4, 0xb9, 0x6e, 0x23, 0x8d, 0xc0, 0x17,
0x5a, 0x06, 0x4b, 0x9c, 0xd1, 0x7f, 0x32, 0xe5, 0xa8]  # the rest of your CrcTable values

def CalCRC8(p):
    crc = 0
    for val in p:
        crc = CrcTable[(crc ^ val) & 0xff]
    return crc

def AssignValues(package):
    frame = {
        'header': package[0],
        'ver_len': package[1],
        'speed': (package[3] << 8 | package[2]),
        'start_angle': (package[5] << 8 | package[4]) / 100,
        'end_angle': (package[43] << 8 | package[42]) / 100,
        'timestamp': (package[45] << 8 | package[44]),
        'object_within_2m': 0,
        'crc8': package[46],
        'point': [None] * POINT_PER_PACK
    }

    distance_average = 0
    distance_count = 0
    angle_step = (frame['end_angle'] - frame['start_angle']) / 11

    for i in range(12):
        distance = (package[8 + i * 3 - 1] << 8 | package[8 + i * 3 - 2])
        confidence = package[8 + i * 3]
        angle = frame['start_angle'] + angle_step * i

        frame['point'][i] = {'distance': distance, 'angle': angle, 'confidence': confidence}

        if confidence >= 180:
            distance_average += distance
            distance_count += 1

            if 0 <= angle <= 15 or 345 <= angle <= 360:
                if distance <= 200:
                    frame['object_within_2m'] = 1
                    print(f"Object within 2m detected: Point {i}, Distance: {distance}, Angle = {angle}")

    return frame

def main():
    counter = 0
    ser = serial.Serial(port='ttyUSB0', baudrate=230400, bytesize=8, parity='N', stopbits=1, timeout=1)

    package = [0] * PACKAGE_SIZE
    packageIndex = 0

    while True:
        global power_val
        key=readkey()
        if key=='6':
            if power_val <=90:
                power_val += 10
                print("power_val:",power_val)
        elif key=='4':
            if power_val >=10:
                power_val -= 10
                print("power_val:",power_val)
        if key=='w':
            fc.forward(power_val)
        elif key=='a':
            fc.turn_left(power_val)
        elif key=='s':
            fc.backward(power_val)
        elif key=='d':
            fc.turn_right(power_val)
        else:
            fc.stop()
        if key=='q':
            print("quit")
            break

        data = ser.read(256)
        if len(data) == 0:
            print("No data read from the serial port")
            continue

        for byte in data:
            if packageIndex == 0 and byte != HEADER:
                continue

            package[packageIndex] = byte
            packageIndex += 1

            if packageIndex == PACKAGE_SIZE:
                crc = CalCRC8(package[:PACKAGE_SIZE - 1])
                if crc == package[PACKAGE_SIZE - 1]:
                    frame = AssignValues(package)
                    if frame['object_within_2m']:
                        counter += 1
                        fc.stop()
                        if counter == threshold:
                            print("Warning! Object detected within 2 meters!")
                    else:
                        counter = 0

                    packageIndex = 0
                else:
                    packageIndex = 0

if __name__ == "__main__":
    main()