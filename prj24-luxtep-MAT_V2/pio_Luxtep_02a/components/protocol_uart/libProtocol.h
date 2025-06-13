#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

//////////////////////////////////////////////////////////////
//  Protocol - SYNC WORDS
//////////////////////////////////////////////////////////////
#define HEADER_SYNC (0xFF)
#define TAIL_SYNC (0xFE)
#define VALID_VAL_MAX (0xEF)

//////////////////////////////////////////////////////////////
//  Protocol - COMMON HEADER 8 Bytes
//////////////////////////////////////////////////////////////
typedef enum {
    IDX_HEADER_0,     //	0xFF
    IDX_HEADER_1,     //	0xFF
    IDX_VER,          //    Ver : 0x02,  25.01.20
    IDX_TX_BOARD_ID,  //    BOARD ID

    IDX_GROUP_ID = 4,  //   COMMON
    IDX_MSG_ID = 5,    //   COMMON

    IDX_DATA_0 = 6,  // NORMAL
    IDX_DATA_1 = 7,  // NORMAL

    IDX_PERMIT_ID = 6,  // G_DEVICE_IO, M_PERMIT

    IDX_HEIGHT = 6,  // G_SENSOR_DATA HEIGHT
    IDX_WIDTH = 7,   // G_SENSOR_DATA WIDTH

    IDX_OSD_ID = 5,      // G_OSD_COMMAND, Left 4bit(osd id) | Right 4bit(board id)
    IDX_LENGTH_100 = 6,  // G_OSD_COMMAND, OSD LENGTH / 100 (SubHeader + Body, No Tail)
    IDX_LENGTH_1 = 7,    // G_OSD_COMMAND, OSD LENGTH % 100 (SubHeader + Body, No Tail)
} PACKET_HEADER_OFFSET;

#define HEAD_LEN 8
#define TAIL_LEN 2

//---------------------------------------------
//  Protocol - SENSOR 1 Board
#define NUM_1Bd_WIDTH 28
#define NUM_1Bd_HEIGHT 34
#define NUM_1Bd_SENSOR_CH (NUM_1Bd_WIDTH * NUM_1Bd_HEIGHT)

#define PACKET_LEN_SEN_1Bd_BODY NUM_1Bd_SENSOR_CH
#define PACKET_LEN_SEN_1Bd (HEAD_LEN + PACKET_LEN_SEN_1Bd_BODY + TAIL_LEN)

//---------------------------------------------
//  Protocol - SENSOR 1SET
#define NUM_1SET_SEN_WIDTH 56
#define NUM_1SET_SEN_HEIGHT 34

#define PACKET_LEN_SEN_1SET_BODY (NUM_1SET_SEN_WIDTH * NUM_1SET_SEN_HEIGHT)
#define PACKET_LEN_SEN_1SET (HEAD_LEN + PACKET_LEN_SEN_1SET_BODY + TAIL_LEN)

//---------------------------------------------
//  Protocol - OSD 1 Board
#define NUM_OSD_1Bd_WIDTH 28
#define NUM_OSD_1Bd_HEIGHT 35

#define PACKET_LEN_OSD_BODY (NUM_OSD_1Bd_WIDTH * NUM_OSD_1Bd_HEIGHT)
#define PACKET_LEN_OSD (HEAD_LEN + SUB_HEAD_LEN + PACKET_LEN_OSD_BODY + TAIL_LEN)

//////////////////////////////////////////////////////////////
//	GROUP ID
//////////////////////////////////////////////////////////////
typedef enum {
    G_DEVICE_IO = 0,
    G_APP_COMMAND,
    G_SENSOR_DATA,
    G_OSD_COMMAND,  // old name : G_LED_COMMAND
} GROUP_ID;

//////////////////////////////////////////////////////////////
//	MESSAGE ID
//////////////////////////////////////////////////////////////
typedef enum {
    M_PERMIT = 0,
} MG_DEVICE_IO;

//---------------------------------------------
//      GROUP ID    : MG_APP_COMMAND
//      MSG ID      
typedef enum {
    M_AC_WIFI_AP_CONFIG_REQ = 0,    // request to owner
    M_AC_WIFI_AP_CONFIG_INFO,       // info from owner
    M_AC_WIFI_AP_CONFIG_INFO_ACK,   // ack from device, 1: OK, 0: FAIL
} MG_APP_COMMAND;

//---------------------------------------------
//      GROUP ID    : MG_SENSOR_DATA, MG_LED_COMMAND
//      MSG ID      : M_BOARD_0 ~ M_BOARD_7
typedef enum {
    M_BOARD_0 = 0,
    M_BOARD_1,
    M_BOARD_2,
    M_BOARD_3,
    M_BOARD_4,
    M_BOARD_5,
    M_BOARD_6,
    M_BOARD_7,
    M_BOARD_LAST_INDEX = M_BOARD_7,
    M_BOARD_ALL,
    M_APP_OWNER,
} MG_SENSOR_DATA,
    MG_OSD_COMMAND;

//---------------------------------------------
//  SUB PACKET HEADER - OSD 1 Board
//      GROUP ID    : G_OSD_COMMAND,
//      MSG ID      : M_BOARD_0 ~ M_BOARD_7
typedef enum {
    IDX_OSD_START_X = 0,
    IDX_OSD_START_Y,
    IDX_OSD_WIDTH,
    IDX_OSD_HEIGHT,
    IDX_DURATION,
    IDX_BRIGHT,
    IDX_RES_SUB0,
    IDX_RES_SUB1,        // 7
    OSD_SUB_HEADER_LEN,  // 8
} SUB_HEADER_OSD_OFFSET;

#define SUB_HEAD_LEN 8

#endif  // _PROTOCOL_H_