/*
LiDAR Data Code
Last Modifed: 4/23/2023
*/

#include <stdio.h>
#include <stdint.h>
#include <Windows.h>

#define POINT_PER_PACK 12
#define PACKAGE_SIZE 47
#define HEADER 0x54

//Data struct for the values we care about regarding measurement inputs from the LiDAR
typedef struct {
    uint16_t distance; //change to float for measurements
    uint8_t intensity;
    float angle;
    uint16_t confidence;
} LidarPointStructDef;

typedef struct {
    float distance;
    uint8_t intensity;
    uint8_t header;
    uint8_t ver_len;
    uint16_t speed;
    uint16_t start_angle;
    LidarPointStructDef point[POINT_PER_PACK];
    uint16_t end_angle;
    uint16_t timestamp;
    uint8_t crc8;
    uint8_t object_within_2m;
} LiDARFrameTypeDef;

static const uint8_t CrcTable[256] = {
0x00, 0x4d, 0x9a, 0xd7, 0x79, 0x34, 0xe3,
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
0x5a, 0x06, 0x4b, 0x9c, 0xd1, 0x7f, 0x32, 0xe5, 0xa8

};

uint8_t CalCRC8(uint8_t* p, uint8_t len) {
    uint8_t crc = 0;
    uint16_t i;
    for (i = 0; i < len; i++) {
        crc = CrcTable[(crc ^ *p++) & 0xff];
    }
    return crc;
}
//
LiDARFrameTypeDef AssignValues(uint8_t package[]) {

    LiDARFrameTypeDef frame = { 0 };
    uint32_t distance_average = 0;
    uint8_t distance_count = 0;

    //From the manual for our model LiDAR LD19 the order of data from the frame is,
    //(len:1byte)header, 
    //(len:1byte)verlen fixed at 0x2C for 001 01100 for packet type(1) and # of measurements per packet(12),
    //(len:2byte)speed, unit degrees/sec
    //(len:2byte)start angle, unit 0.01 degrees
    //(len:3byte)data, 
    //(len:2byte)end angle, unit 0.01 degrees
    //(len:2byte)timestamp, unit milliseconds, max 30000
    frame.header = package[0];

    frame.ver_len = package[1];

    frame.speed = (package[3] << 8 | package[2]);

    frame.start_angle = (package[5] << 8 | package[4]) / 100;

    frame.end_angle = (package[43] << 8 | package[42]) / 100; //adding because end angle is 8 or 9 degrees

    frame.timestamp = (package[45] << 8 | package[44]);

    frame.object_within_2m = 0;

    frame.crc8 = package[46];

    float angle_step = (frame.end_angle - frame.start_angle) / 11;
    for (int i = 0; i < 12; i++) {
        frame.point[i].angle = frame.start_angle + angle_step * (i);
        frame.point[i].distance = (package[8 + i * 3 - 1] << 8 | package[8 + i * 3 - 2]);
        frame.point[i].confidence = package[8 + i * 3];

        if (frame.point[i].confidence >= 180)
        {
            distance_average += frame.point[i].distance;
            distance_count++;


            if (0 <= (int)frame.point[i].angle && frame.point[i].angle <= 30) {

                if (frame.point[i].distance <= 200) { // Check if the object is within 2 meters
                    frame.object_within_2m = 1;
                    printf("Object within 2m detected: Point %d, Distance: %d, Angle = %f\n", i, frame.point[i].distance, frame.point[i].angle); // Debugging print statement
                }

            }




        }

    }



    return frame;
}

int main()
{
    //below is the code for opening and validating the serial port for the lidar detector. IT seems that the LD19 defaults to "COM4"
    HANDLE hSerial = CreateFileW(L"COM5", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hSerial == INVALID_HANDLE_VALUE)
    {
        printf("Error opening serial port\n");
        return 1;
    }

    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hSerial, &dcbSerialParams))
    {
        printf("Error getting serial port parameters\n");
        CloseHandle(hSerial);
        return 1;
    }

    dcbSerialParams.BaudRate = 230400; //Default Lidar Baudrate is 115200, our LD19 is capabale of 230400
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.Parity = NOPARITY;
    dcbSerialParams.StopBits = ONESTOPBIT;

    if (!SetCommState(hSerial, &dcbSerialParams))
    {
        printf("Error setting serial port parameters\n");
        CloseHandle(hSerial);
        return 1;
    }

    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.ReadTotalTimeoutConstant = 100;

    if (!SetCommTimeouts(hSerial, &timeouts))
    {
        printf("Error setting serial port timeouts\n");
        CloseHandle(hSerial);
        return 1;
    }


    uint8_t buffer[256];//256bytes of data are stored in the buffer
    uint8_t package[47];
    int packageIndex = 0;
    DWORD bytesRead = 0;

    while (true)
    {
        if (!ReadFile(hSerial, buffer, sizeof(buffer), &bytesRead, NULL))
        {
            printf("Error reading from serial port\n");
            CloseHandle(hSerial);
            return 1;
        }

        if (bytesRead > 0)
        {
            for (int i = 0; i < bytesRead; i++)
            {
                if (packageIndex == 0 && buffer[i] != HEADER)
                {
                    continue;
                }

                package[packageIndex++] = buffer[i];

                if (packageIndex == PACKAGE_SIZE)
                {
                    uint8_t crc = CalCRC8(package, PACKAGE_SIZE - 1);
                    if (crc == package[PACKAGE_SIZE - 1]) {
                        LiDARFrameTypeDef frame = AssignValues(package);


                        for (int j = 0; j < POINT_PER_PACK; j++) {




                            printf("Point %d: Distance: %d, Angle: %.2f, Confidence: %d\n",
                                j, frame.point[j].distance, frame.point[j].angle, frame.point[j].confidence);



                        }

                        if (frame.object_within_2m) {
                            printf("Warning! Object detected within 2 meters!\n");
                        }
                        printf("\n");
                        packageIndex = 0;
                    }
                    else {
                        packageIndex = 0;
                    }
                }
            }
        }
    }

    CloseHandle(hSerial);

    return 0;
}