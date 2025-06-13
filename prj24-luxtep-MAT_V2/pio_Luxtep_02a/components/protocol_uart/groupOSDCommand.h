#ifndef _GROUP_LED_COMMAND_H_
#define _GROUP_LED_COMMAND_H_

#include "packetBuffer.h"
#include "lib_ledWorks_28x35.h"

extern byte *pMyOSDBuf;

extern int indi_1_r;
extern int indi_1_g;
extern int indi_1_b;

extern void printUart1(const char *fmt, ...);
extern bool sendPacket1(byte *packet_buffer, int packet_len);

int osd_test_count = 0;


void copyPacketToOSDBuf(int my_board_id, byte *rx_packet_header, byte *rx_packet_body) {
    int msg_id = rx_packet_header[IDX_MSG_ID];
    int osd_buf_id = (msg_id & 0xF0) >> 4;

    OSD_START_X = rx_packet_body[IDX_OSD_START_X];
    OSD_START_Y = rx_packet_body[IDX_OSD_START_Y];
    OSD_WIDTH = rx_packet_body[IDX_OSD_WIDTH];
    OSD_HEIGHT = rx_packet_body[IDX_OSD_HEIGHT];

    //  FILL packet to the OSD buffer
    byte * packet_osd_data = rx_packet_body + SUB_HEAD_LEN;


    memset(pMyOSDBuf, 0xEF, PACKET_LEN_OSD_BODY);  // clear the packet buffer

    int data_offset = 0;
    for (int y = OSD_START_Y; y < (OSD_START_Y + OSD_HEIGHT); y++) {
        int led_index = y * NUM_OSD_1Bd_WIDTH + OSD_START_X;

        memcpy(pMyOSDBuf + led_index, packet_osd_data + data_offset, OSD_WIDTH);
        data_offset += OSD_WIDTH;
    }
    uart0_printf("o");

    printUart1("#Copy OSD x=%d, y=%d, w=%d, h=%d\n", OSD_START_X, OSD_START_Y, OSD_WIDTH, OSD_HEIGHT);

    osd_test_count++;

    return;
}

void buildPacket_OSD_1Bd(int my_board_id, byte *osd_packet_buffer, int start_x, int start_y, int width, int height) {
    int header_len = 8;
    int sub_header_len = 8;
    int tail_len = 2;
    int data_len = width * height;
    int packet_body_len = sub_header_len + data_len + tail_len;  
    
    osd_packet_buffer[IDX_HEADER_0] = HEADER_SYNC;     // 0xFF
    osd_packet_buffer[IDX_HEADER_1] = HEADER_SYNC;     // 0xFF
    osd_packet_buffer[IDX_VER] = osd_test_count % 200;                 // Major Ver OR debugging index
    osd_packet_buffer[IDX_TX_BOARD_ID] = 0;  // BOARD ID contained in this PACKET, M_BOARD_ALL = 8

    osd_packet_buffer[IDX_GROUP_ID] = G_OSD_COMMAND;  // GROUP ID
    osd_packet_buffer[IDX_MSG_ID] = (0 << 4) | 0;      // MSG ID, osd_id << 4 | rx_board_id 
    osd_packet_buffer[IDX_LENGTH_100] = packet_body_len / 100;           // height = 34
    osd_packet_buffer[IDX_LENGTH_1] = packet_body_len % 100;             // width = 56

    osd_packet_buffer[header_len + IDX_OSD_START_X] = start_x;
    osd_packet_buffer[header_len + IDX_OSD_START_Y] = start_y;
    osd_packet_buffer[header_len + IDX_OSD_WIDTH] = width;
    osd_packet_buffer[header_len + IDX_OSD_HEIGHT] = height;

    osd_packet_buffer[12] = -17; // duration, 100ms. Doesn't work yet
    osd_packet_buffer[13] = 100; // brightness. Doesn't work yet
    osd_packet_buffer[14] = 0; // res 0
    osd_packet_buffer[15] = 0; // res 1

    int pa_index = header_len + sub_header_len;  // 8 + 8 = 16
    byte * packet_osd_data = osd_packet_buffer + pa_index;

    // memset(packet_osd_data + width * 24, 10, width * 10);  // clear the packet buffer

    for (int y = 0 ; y < height ; y++) {
        int data_offset = y * width;
        int color_val = 12 + y * 2;

        //  packet offset : 0, 1, 2, 
        //  osd coordinate : start_x, start_x + 1, start_x + 2
        memset(packet_osd_data + data_offset, color_val, width);
    }
    pa_index += width * height;

    osd_packet_buffer[pa_index++] = 0;          //  Reserved 2
    osd_packet_buffer[pa_index++] = TAIL_SYNC;  // 0xFE

}


int parsePacket_OSD_byMain(int my_board_id, byte *rx_packet_header, byte *rx_packet_body, bool &is_osd_to_send) {
    //---------------------------------------
    // 1단계: 헤더 해석 ==> 읽을 사이즈 결정
    int tx_board_id = rx_packet_header[IDX_TX_BOARD_ID];
    int msg_id = rx_packet_header[IDX_MSG_ID];
    int osd_buf_id = (msg_id & 0xF0) >> 4;
    int rx_board_id = (msg_id & 0x0F);

    int size_100 = rx_packet_header[IDX_LENGTH_100];  // OSD LENGTH / 100 (SubHeader + Body, No Tail)
    int size_1 = rx_packet_header[IDX_LENGTH_1];      // OSD LENGTH % 100 (SubHeader + Body, No Tail)
    int body_len = size_100 * 100 + size_1;

    //---------------------------------------
    // 2단계: 수신 - 서브 헤더 및 테일
    // int to_read = OSD_SUB_HEADER_LEN + body_len + TAIL_LEN;
    int to_read = body_len;
    int read_len = 0;

    // uart0_printf("Main parse OSD Bd=%d, Len=%d.\n", rx_board_id, body_len);
    // printUart1("#Main parse OSD Bd=%d, Len=%d, [-1]=%d (w=%d,h=%d)\n", rx_board_id, body_len, 
    //             rx_packet_body[body_len - 1],
    //             rx_packet_body[IDX_OSD_WIDTH], rx_packet_body[IDX_OSD_HEIGHT]);

    //---------------------------------------
    // 3단계: 보관 - 전송 혹은 보관

    //  send this packet to rx_board_id ----------------------------------
    if (my_board_id != rx_board_id) {
        // memcpy(rx_packet_body, rx_packet_header, HEAD_LEN);  // body_len

        //  packet is already at packetBuf_OSDSub

        is_osd_to_send = true;
        // sendPacket1(rx_packet_body, PACKET_LEN_OSD);

    } else {
        copyPacketToOSDBuf(my_board_id, rx_packet_header, rx_packet_body);
    }

    return 0;
}


int parsePacket_OSD_bySub(int my_board_id, byte *rx_packet_header, byte *rx_packet_body) {
    //---------------------------------------
    // 1단계: 헤더 해석 ==> 읽을 사이즈 결정
    int tx_board_id = rx_packet_header[IDX_TX_BOARD_ID];
    int msg_id = rx_packet_header[IDX_MSG_ID];
    int osd_buf_id = (msg_id & 0xF0) >> 4;
    int rx_board_id = (msg_id & 0x0F);

    int size_100 = rx_packet_header[IDX_LENGTH_100];  // OSD LENGTH / 100 (SubHeader + Body, No Tail)
    int size_1 = rx_packet_header[IDX_LENGTH_1];      // OSD LENGTH % 100 (SubHeader + Body, No Tail)
    int body_len = size_100 * 100 + size_1;

    //---------------------------------------
    // 2단계: 서브 헤더 및 테일 읽기
    // int to_read = body_len + TAIL_LEN;
    int to_read = body_len;
    int read_len = 0;

    
    // if (my_board_id != rx_board_id) {
    //     return -10;
    // }

    //---------------------------------------
    // 3단계: 서브 헤더 파싱
    copyPacketToOSDBuf(my_board_id, rx_packet_header, (rx_packet_body));

    return 0;
}
#endif  // _GROUP_LED_COMMAND_H_