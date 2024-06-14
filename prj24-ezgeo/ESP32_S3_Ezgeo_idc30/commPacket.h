/*
  <Arduino Config>
  BOARD : "ESP32-S3 DEVKITC-1" (NOT 1.1) OR "ADAFRUIT QT py esp32-C3"
  DESC : SRAM 512K, ROM 384K, FLASH 16M, PSRAM 8M
  CONFIG : Flash QIO 80MHz, OPI PSRAM, Partition Minimal SPIFFS, others : apply default setting

  SEE 74HC595 later - https://docs.arduino.cc/tutorials/communication/guide-to-shift-out
*/
#include "configPins.h"
#include "lib_rle_util.h"
// #include "lib_rle.h"

//---------------------------------------------
//  Serial config
#define SERIAL_SIZE_TX 512   // used in Serial.setRxBufferSize()
#define SERIAL_SIZE_RX 1024  // used in Serial.setRxBufferSize()

#define BAUD_RATE 230400

//---------------------------------------------
//  Protocol - Packet
#define HEADER_LEN 8
#define TAIL_LEN 2

#define PACKET_BODY_LEN (NUM_USED_OUT * MUX_LIST_LEN)
#define PACKET_LEN (HEADER_LEN + PACKET_BODY_LEN + TAIL_LEN)

byte packetBuf[PACKET_LEN];

#define IDX_HEADER_0 0
#define IDX_HEADER_1 1
#define IDX_MAJOR_VER 2
#define IDX_MINOR_VER 3
#define IDX_BODY_LEN 4
#define IDX_BOARD_ID 5  // TOP(UART MASTER) : 0, BOTTOM(UART SLAVE) : 1
#define IDX_RES_0 6
#define IDX_RES_1 7

#define IDX_TAIL_0 (PACKET_LEN - 2)
#define IDX_TAIL_1 (PACKET_LEN - 1)

//---------------------------------------------
//  Protocol - Parcel
#define HEADER_SYNC (0xFF)
#define TAIL_SYNC (0xFE)
#define VALID_VAL_MAX (0xEF)

//---------------------------------------------
//  Buffer for RLE
#define PACKET_ENC_LEN (PACKET_LEN + 50)  // 50 : protect rle encoding overflow
byte packetBufEnc[PACKET_ENC_LEN];

#define PACKET_DEC_LEN PACKET_ENC_LEN
byte packetBufDec[PACKET_LEN];  // for test enc/dec

byte packetBufSlave[PACKET_LEN * 5];  // * 5 for overflow
int deliver_count_slave = 0;

#define BOARD_ID_TOP 0
#define BOARD_ID_BOTTOM 1

#define BOARD_ID BOARD_ID_TOP  // TOP(UART MASTER) : 0, BOTTOM(UART SLAVE) : 1

//---------------------------------------------
//  Function Declarations
//---------------------------------------------
void sendPacket(byte *packet_buffer, int packet_len);
void sendBLE(byte *packet_buffer, int packet_len);
void sendBLEGrib(byte *packet_raw_buffer, int packet_len);

//---------------------------------------------
//  Function Definitions
//---------------------------------------------

byte trimVal8(byte raw_value) {
    byte value = 0;
    if (255 == raw_value) {
        value = VALID_VAL_MAX;
    } else if ((VALID_VAL_MAX - 1) <= raw_value) {
        value = VALID_VAL_MAX - 1;
    } else if (raw_value < 0) {
        value = 0;
    } else {
        value = raw_value;
    }

    return value;
}

void buildPacket(byte *packet_buffer, int adc_mat_buf[MUX_LIST_LEN][NUM_MUX_OUT], int num_line, int line_len) {
    packet_buffer[0] = HEADER_SYNC;       // 0xFF
    packet_buffer[1] = HEADER_SYNC;       // 0xFF
    packet_buffer[2] = 0x01;              // Major Ver
    packet_buffer[3] = 0x00;              // Minor Ver
    packet_buffer[4] = 0;  // Packet body Len, width, height,
    packet_buffer[5] = num_line;          // Board ID
    packet_buffer[6] = line_len;              // Reserved 0
    packet_buffer[7] = 0x00;              // Reserved 1

    int pa_index = HEADER_LEN;

    //  ezgeo data len = 5 x 18 = 90
    //  valid data is 4 lines = 16 x 4 = 64

    for (int w = 0; w < num_line; w++) {
        for (int h = 0; h < line_len; h++) {
            packet_buffer[pa_index++] = trimVal8(adc_mat_buf[w][h]);
        }
    }

    packet_buffer[pa_index++] = 0x00;       //  Reserved 2
    packet_buffer[pa_index++] = TAIL_SYNC;  // 0xFE
}

void buildPacket_EzgeoTest(byte *packet_buffer, int adc_mat_buf[MUX_LIST_LEN][NUM_MUX_OUT], int width, int height) { // height = 16
    packet_buffer[0] = HEADER_SYNC;       // 0xFF
    packet_buffer[1] = HEADER_SYNC;       // 0xFF
    packet_buffer[2] = 0x01;              // Major Ver
    packet_buffer[3] = 0x00;              // Minor Ver
    packet_buffer[4] = 0;                 // board id
    packet_buffer[5] = 18;                // row = 18
    packet_buffer[6] = 5;                 // column = 5
    packet_buffer[7] = 0x00;              // Reserved 1

    int pa_index = HEADER_LEN;

    //  ezgeo data len = 5 x 18 = 90
    //  valid data is 4 lines = 16 x 4 = 64

    for (int w = 0; w < width; w++) {
        for (int h = 0; h < height; h++) {
            packet_buffer[pa_index++] = trimVal8(adc_mat_buf[w][h]);
        }
    }

    pa_index = HEADER_LEN + 90;

    packet_buffer[pa_index++] = TAIL_SYNC;       //  Reserved 2
    packet_buffer[pa_index++] = TAIL_SYNC;  // 0xFE
}


void tempDelay(int time_len_ms) {
    delay(time_len_ms);
}
