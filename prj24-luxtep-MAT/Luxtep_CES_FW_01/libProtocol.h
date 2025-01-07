#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

// #include "commPacket.h"
//---------------------------------------------
//  Protocol - COMMON
//---------------------------------------------

#define IDX_HEAD_0 0         //	0xFF
#define IDX_HEAD_1 1         //	0xFF
#define IDX_VERSION_MAJOR 2  // 0x01
#define IDX_VERSION_MINOR 3  // 0x00
#define IDX_GROUP_ID 4       // height
#define IDX_MSG_ID 5         // width
#define IDX_LENGTH_100 6     // PACKET BODY LENGTH / 100
#define IDX_LENGTH_0 7       // PACKET BODY LENGTH % 100
#define PACKET_HEAD_LEN 8
#define PACKET_TAIL_LEN 2

#define TAIL_VALUE 0xFE

//---------------------------------------------
//  SUB PACKET
//---------------------------------------------
#define IDX_OSD_START_X 9
#define IDX_OSD_START_Y 10
#define IDX_OSD_WIDTH 11
#define IDX_OSD_HEIGHT 12
#define IDX_DURATION 13
#define IDX_BRIGHT 14
#define IDX_RES_0 15
#define IDX_RES_1 16

#define SUB_HEAD_LEN 8

//---------------------------------------------
//	GROUP ID
//---------------------------------------------
typedef enum {
    G_DEVICE_IO = 0,
    G_APP_COMMAND,
    G_SENSOR_DATA,
    G_LED_COMMAND,
} GROUP_ID;

//---------------------------------------------
//	MESSAGE ID
//---------------------------------------------
typedef enum {
    M_BOARD_0 = 0,
    M_BOARD_1,
    M_BOARD_2,
    M_BOARD_3,
    M_BOARD_4,
    M_BOARD_5,
    M_BOARD_6,
    M_BOARD_7,
} M_G2,
    M_G3;

//---------------------------------------------
//	COLOR TABLE
//  Web Safety Color 216
//  https://edyoon.tistory.com/1069
//  int rgb_8bit = (r / 51) * 36 + (g / 51) * 6 + b / 51;
//---------------------------------------------
void rgb_8to24(byte rgb_8bit, byte &r, byte &g, byte &b) {
    r = (rgb_8bit / 36) * 51;
    int r_reminder = rgb_8bit % 36;

    g = (r_reminder / 6) * 51;
    b = (r_reminder % 6) * 51;
}

byte rgb_24to8(int r, int g, int b) {
    int rgb_8bit = (r / 51) * 36 + (g / 51) * 6 + b / 51;
    return rgb_8bit;
}

//---------------------------------------------
//
//---------------------------------------------

#endif  // _PROTOCOL_H_