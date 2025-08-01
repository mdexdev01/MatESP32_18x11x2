#ifndef _GROUP_SENSOR_DATA_H_
#define _GROUP_SENSOR_DATA_H_

#include "groupDeviceIO.h"
#include "packetBuffer.h"

int build_count = 0;
extern bool sendPacket1(byte *packet_buffer, int packet_len);

void buildPacket_Sensor_1Set(int my_board_id, byte *tx_packet_buffer, byte *adc_buffer, int width, int height) {
    tx_packet_buffer[IDX_HEADER_0] = HEADER_SYNC;     // 0xFF
    tx_packet_buffer[IDX_HEADER_1] = HEADER_SYNC;     // 0xFF
    tx_packet_buffer[IDX_VER] = 0x02;                 // Major Ver
    tx_packet_buffer[IDX_TX_BOARD_ID] = my_board_id;  // BOARD ID contained in this PACKET, M_BOARD_ALL = 8

    tx_packet_buffer[IDX_GROUP_ID] = G_SENSOR_DATA;  // GROUP ID
    tx_packet_buffer[IDX_MSG_ID] = M_BOARD_ALL;      // MSG ID
    tx_packet_buffer[IDX_HEIGHT] = height;           // height = 34
    tx_packet_buffer[IDX_WIDTH] = width;             // width = 56

    // tx_packet_buffer[4] = NUM_1SET_SEN_HEIGHT;  // row = height = 12
    // tx_packet_buffer[5] = NUM_1SET_SEN_WIDTH;   // column = width = 10
    // tx_packet_buffer[6] = 0;                    // Reserved 0
    // tx_packet_buffer[7] = build_count % 0xEF;   // Reserved 1

    int pa_index = HEAD_LEN;

    int adc_offset_main = 0;
    int adc_offset_sub = 0;
    for (int y = 0; y < NUM_1SET_SEN_HEIGHT; y++) {     // NUM_1SET_SEN_HEIGHT = 34
        for (int x = 0; x < NUM_1SET_SEN_WIDTH; x++) {  // NUM_1SET_SEN_WIDTH = 56
            int luxtep_index = y * NUM_1SET_SEN_WIDTH + x;

            //	board 1 : left (x = 0 ~ SIZE_X)
            if ((luxtep_index % NUM_1SET_SEN_WIDTH) < SIZE_X) {
                tx_packet_buffer[pa_index] = forceBuffer_bd1[adc_offset_sub++];
            }
            //	board 0 : right (x = SIZE_X ~ NUM_1SET_SEN_WIDTH)
            else {
                // tx_packet_buffer[pa_index] = adc_mat_buf[x - SIZE_X][y];
                // tx_packet_buffer[pa_index] = forceBuffer_rd[adc_offset_main++];
                tx_packet_buffer[pa_index] = adc_buffer[adc_offset_main++];
            }

            pa_index++;
        }
    }

    tx_packet_buffer[pa_index++] = build_count % 100;          //  Reserved 2
    tx_packet_buffer[pa_index++] = TAIL_SYNC;  // 0xFE

    build_count++;
}

void buildPacket_Sensor_1Bd(int my_board_id, byte *tx_packet_buffer, byte *adc_buffer, int width, int height) {
    tx_packet_buffer[IDX_HEADER_0] = HEADER_SYNC;  // 0xFF
    tx_packet_buffer[IDX_HEADER_1] = HEADER_SYNC;  // 0xFF
    // tx_packet_buffer[IDX_VER] = 0x02;              // Major Ver
    tx_packet_buffer[IDX_VER] = build_count % 100;    // Major Ver
    // tx_packet_buffer[IDX_VER] = indexPermit;  // Major Ver

    tx_packet_buffer[IDX_TX_BOARD_ID] = my_board_id;  // BOARD ID owns this PACKET

    tx_packet_buffer[IDX_GROUP_ID] = G_SENSOR_DATA;  // GROUP ID
    tx_packet_buffer[IDX_MSG_ID] = M_BOARD_0;        // MSG ID
    tx_packet_buffer[IDX_HEIGHT] = height;           //
    tx_packet_buffer[IDX_WIDTH] = width;             //

    int body_len = height * width;

    int pa_index = HEAD_LEN;

    int adc_offset_sub = 0;
    // for (int y = 0; y < height; y++) {
    //     for (int x = 0; x < width; x++) {
    //         int luxtep_index = y * width + x;

    //         // tx_packet_buffer[pa_index] = forceBuffer_rd[adc_offset_sub++];
    //         tx_packet_buffer[pa_index] = adc_mat_buf[x][y];

    //         pa_index++;
    //     }
    // }

    memcpy(tx_packet_buffer + HEAD_LEN, adc_buffer, body_len);

    pa_index = HEAD_LEN + (height * width);

    tx_packet_buffer[pa_index++] = build_count % 100;          //  Reserved 2
    tx_packet_buffer[pa_index++] = TAIL_SYNC;  // 0xFE

    //  build this packet while led is drawing. So the below comment can show the jitter.
    // uart0_printf("build - forceBuffer_rd check : [0]%d, { [10]%d, [50]%d, [900]%d}, [961]%d \n",
    //              forceBuffer_rd[0], forceBuffer_rd[10], forceBuffer_rd[50], forceBuffer_rd[900], forceBuffer_rd[961]);

    // uart0_printf("build - tx_packet_buffer: [0]%d, { [10]%d, [50]%d, [900]%d}, [961]%d \n",
    //              tx_packet_buffer[0], tx_packet_buffer[10], tx_packet_buffer[50], tx_packet_buffer[900], tx_packet_buffer[961]);

    build_count++;
}

int parsePacket_Sensor_1Bd(int my_board_id, byte *rx_packet_header, byte *rx_packet_body, int &tx_board_id) {
    //---------------------------------------
    // 1단계: 헤더 해석 ==> 읽을 사이즈 결정
    // int tx_board_id = rx_packet_header[IDX_TX_BOARD_ID];
    tx_board_id = rx_packet_header[IDX_TX_BOARD_ID];
    int height = rx_packet_header[IDX_HEIGHT];  // BOARD ID permitted
    int width = rx_packet_header[IDX_WIDTH];    // Reserved 1
    int body_len = height * width;

    //---------------------------------------
    // 2단계: 서브 헤더 및 테일 읽기
    int to_read = body_len + TAIL_LEN;
    int read_len = 0;

    // uart_read_bytes(UART_NUM_1, rx_packet_body, body_len + TAIL_LEN, OS_500ms);
    if (my_board_id == 0) {
        memcpy(forceBuffer_bd1, rx_packet_body, body_len);  /// ==> 이러면 됨.
    }

    // uart0_printf("[%8d] RX1 OK, Sen{%d},[2]%2d\n", millis(), tx_board_id, rx_packet_header[2]);

    return 0;
}

void printHexa(byte *hex_data, int hex_len) {
    for (int i = 0; i < hex_len; i++) {
        uart0_printf("%02x", hex_data[i]);
    }
}

#endif  // _GROUP_SENSOR_DATA_H_