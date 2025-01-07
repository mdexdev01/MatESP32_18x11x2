//---------------------------------------------
//  Protocol - COMMON
//---------------------------------------------

final int IDX_HEAD_0 = 0;         //  0xFF
final int IDX_HEAD_1 = 1;         //  0xFF
final int IDX_VERSION_MAJOR = 2;  // 0x01
final int IDX_VERSION_MINOR =3;  // 0x00
final int IDX_GROUP_ID =4;       // height
final int IDX_MSG_ID =5;         // width
final int IDX_LENGTH_100 =6;     // PACKET BODY LENGTH / 100
final int IDX_LENGTH_0 =7;       // PACKET BODY LENGTH % 100
final int PACKET_HEAD_LEN =8;

final int TAIL_VALUE =0xFE;

//---------------------------------------------
//  SUB PACKET
//---------------------------------------------
final int IDX_OSD_START_X =9;
final int IDX_OSD_START_Y =10;
final int IDX_OSD_WIDTH =11;
final int IDX_OSD_HEIGHT =12;
final int IDX_RES_0 =13;
final int IDX_RES_1= 14;

//---------------------------------------------
//  GROUP ID
//---------------------------------------------
enum GROUP_ID{
    G_DEVICE_IO,
    G_APP_COMMAND,
    G_SENSOR_DATA,
    G_LED_COMMAND,
} ;

//---------------------------------------------
//  MESSAGE ID
//---------------------------------------------
enum M_G2{
    M_BOARD_0,
    M_BOARD_1,
    M_BOARD_2,
    M_BOARD_3,
    M_BOARD_4,
    M_BOARD_5,
    M_BOARD_6,
    M_BOARD_7,
};

//---------------------------------------------
//
//---------------------------------------------
