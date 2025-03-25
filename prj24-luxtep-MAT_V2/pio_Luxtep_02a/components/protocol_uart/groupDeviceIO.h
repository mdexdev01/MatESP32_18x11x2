#ifndef _GROUP_DEVICEIO_H_
#define _GROUP_DEVICEIO_H_

#include "packetBuffer.h"

extern int MY_BOARD_ID;
extern int indexPermit;

int reser_1 = 0;
int buildPacket_Permit(byte *tx_packet_buffer, int board_id_permitted) {
    tx_packet_buffer[IDX_HEADER_0] = HEADER_SYNC;   // 0xFF
    tx_packet_buffer[IDX_HEADER_1] = HEADER_SYNC;   // 0xFF
    tx_packet_buffer[IDX_VER] = reser_1;            // Major Ver = 2 (2025-02-20)
    tx_packet_buffer[IDX_TX_BOARD_ID] = M_BOARD_0;  // ONLY M_BOARD_0 can send permission packet

    tx_packet_buffer[IDX_GROUP_ID] = G_DEVICE_IO;          // GROUP ID
    tx_packet_buffer[IDX_MSG_ID] = M_PERMIT;               // MSG ID
    tx_packet_buffer[IDX_PERMIT_ID] = board_id_permitted;  // BOARD ID permitted
    tx_packet_buffer[IDX_DATA_1] = reser_1;                // Reserved 1

    int pa_index = HEAD_LEN;

    tx_packet_buffer[pa_index++] = 0;          //  Reserved 2
    tx_packet_buffer[pa_index++] = TAIL_SYNC;  // 0xFE

    return pa_index;
}

int parsePacket_Permit(byte *rx_packet_header, byte *rx_packet_body, int my_board_id, bool &key_granted) {
    key_granted = false;

    // uart0_printf("SUB RX, permit id = %d \n", rx_packet_body[IDX_PERMIT_ID]);

    if (rx_packet_header[IDX_PERMIT_ID] == my_board_id) {
        // vTaskDelay(1);
        key_granted = true;
    }

    return 0;
}

#endif  // _GROUP_DEVICEIO_H_