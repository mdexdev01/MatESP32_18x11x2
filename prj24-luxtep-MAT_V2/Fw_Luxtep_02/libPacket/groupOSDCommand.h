#ifndef _GROUP_LED_COMMAND_H_
#define _GROUP_LED_COMMAND_H_

#include "packetBuffer.h"

extern byte *pMyOSDBuf;

extern int indi_1_r;
extern int indi_1_g;
extern int indi_1_b;

extern void printUart1(const char *fmt, ...);
extern bool sendPacket1(byte *packet_buffer, int packet_len);

void copyPacketToOSDBuf(int my_board_id, byte *rx_packet_header, byte *rx_packet_body) {
    int msg_id = rx_packet_header[IDX_MSG_ID];
    int osd_buf_id = (msg_id & 0xF0) >> 4;

    osd_start_x = rx_packet_body[IDX_OSD_START_X];
    osd_start_y = rx_packet_body[IDX_OSD_START_Y];
    osd_width = rx_packet_body[IDX_OSD_WIDTH];
    osd_height = rx_packet_body[IDX_OSD_HEIGHT];

    //  FILL packet to the OSD buffer
    int packet_offset = SUB_HEAD_LEN;

    memset(pMyOSDBuf, 0, PACKET_LEN_OSD_BODY);

    for (int y = osd_start_y; y < (osd_start_y + osd_height); y++) {
        for (int x = osd_start_x; x < (osd_start_x + osd_width); x++) {
            int index = y * SIZE_X + x;

            pMyOSDBuf[index] = rx_packet_body[packet_offset++];  // if OSD Buffer is multiple, must see the index.
        }
    }

    return;
}

int parsePacket_OSD_byMain(int my_board_id, byte *rx_packet_header, byte *rx_packet_body, bool &is_osd_to_send) {
    //---------------------------------------
    // 1단계: 헤더 해석 ==> 읽을 사이즈 결정
    int tx_board_id = rx_packet_header[IDX_TX_BOARD_ID];
    int msg_id = rx_packet_header[IDX_MSG_ID];
    int osd_buf_id = (msg_id & 0xF0) >> 4;
    int rx_board_id = (msg_id & 0x0F);

    int size_100 = rx_packet_header[IDX_LENGTH_100];  // OSD LENGTH / 100 (SubHeader + Body, No Tail)
    int size_1 = rx_packet_header[IDX_LENGTH_0];      // OSD LENGTH % 100 (SubHeader + Body, No Tail)
    int body_len = size_100 * 100 + size_1;

    //---------------------------------------
    // 2단계: 수신 - 서브 헤더 및 테일
    // int to_read = OSD_SUB_HEADER_LEN + body_len + TAIL_LEN;
    int to_read = body_len;
    int read_len = 0;

    //  header:8 + subheader:8 + body:980(35x28) + tail:2 = 998
    //  to_read = 990 (after header)

    int lap_enter = millis();
    while (true) {
        // reading. timeout : 0 ==> Asnync reading. Return instantly.  OS_500ms, portMAX_DELAY
        read_len += uart_read_bytes(UART_NUM_0, (rx_packet_body + HEAD_LEN) + read_len, to_read - read_len, 0);
        // read_len += uart_read_bytes(UART_NUM_0, (rx_packet_body + HEAD_LEN) + read_len, to_read - read_len, portMAX_DELAY);

        if (read_len == to_read)
            break;
        if (1000 < (millis() - lap_enter))  // if over 1000ms, escape
            return -1;
        // break;
        vTaskDelay(1);
    }

    if (read_len < to_read) {
        printUart1("[%8d] > ERROR RX0 read shortly, %d/%d \n", millis(), read_len, to_read);
        return -1;
    }

    if (rx_packet_body[to_read + HEAD_LEN - 1] != TAIL_SYNC) {
        printUart1("> ERR 994 : %3d, %3d, %3d, %3d\n",
                   //    rx_packet_body[to_read + HEAD_LEN - 2], rx_packet_body[to_read + HEAD_LEN - 1]);
                   rx_packet_body[994], rx_packet_body[995], rx_packet_body[996], rx_packet_body[997]);
        return -10;
    }

    //---------------------------------------
    // 3단계: 보관 - 전송 혹은 보관

    //  send this packet to rx_board_id ----------------------------------
    if (my_board_id != rx_board_id) {
        //  copy header
        memcpy(rx_packet_body, rx_packet_header, HEAD_LEN);  // body_len

        is_osd_to_send = true;
        // sendPacket1(rx_packet_body, PACKET_LEN_OSD);

    } else {
        copyPacketToOSDBuf(my_board_id, rx_packet_header, (rx_packet_body + HEAD_LEN));
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
    int size_1 = rx_packet_header[IDX_LENGTH_0];      // OSD LENGTH % 100 (SubHeader + Body, No Tail)
    int body_len = size_100 * 100 + size_1;

    //---------------------------------------
    // 2단계: 서브 헤더 및 테일 읽기
    // int to_read = body_len + TAIL_LEN;
    int to_read = body_len;
    int read_len = 0;

    //  header:8 + subheader:8 + body:980(35x28) + tail:2 = 998
    //  to_read = 990 (after header)

    int lap_enter = millis();
    while (true) {
        // reading. timeout : 0 ==> Asnync reading. Return instantly.  OS_500ms, portMAX_DELAY
        read_len += uart_read_bytes(UART_NUM_1, (rx_packet_body + HEAD_LEN) + read_len, to_read - read_len, 0);
        if (read_len == to_read)
            break;
        if (1000 < (millis() - lap_enter))  // if over 1000ms, escape
            break;
        vTaskDelay(1);
    }

    if (read_len < to_read) {
        uart0_printf("[%8d] RX1 OSD has read shortly, len = %d, to read = %d \n", millis(), read_len, to_read);
        return -1;
    }

    if (rx_packet_body[to_read + HEAD_LEN - 1] != TAIL_SYNC) {
        uart0_printf("[%8d] ERROR\t RX OSD ends with ~ %d, %d \n", millis(),
                     rx_packet_body[to_read - 2], rx_packet_body[to_read - 1]);
        return -2;
    }

    if (my_board_id != rx_board_id) {
        return -10;
    }

    //---------------------------------------
    // 3단계: 서브 헤더 파싱
    copyPacketToOSDBuf(my_board_id, rx_packet_header, (rx_packet_body + HEAD_LEN));

    return 0;
}
#endif  // _GROUP_LED_COMMAND_H_