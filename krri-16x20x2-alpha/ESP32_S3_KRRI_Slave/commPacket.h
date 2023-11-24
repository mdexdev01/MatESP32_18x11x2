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

#define PACKET_BODY_LEN (NUM_MUX_OUT * MUX_LIST_LEN)
#define PACKET_LEN (HEADER_LEN + PACKET_BODY_LEN + TAIL_LEN)

byte packetBuf[PACKET_LEN];

//---------------------------------------------
//  Protocol - Parcel
// #define PARCEL_MUX_LEN 6
#define PARCEL_MUX_LEN MUX_LIST_LEN
// #define PACKET_DIV        (MUX_LIST_LEN / PARCEL_MUX_LEN) //  18 / 6
#define PARCEL_BODY_LEN (NUM_MUX_OUT * PARCEL_MUX_LEN)
#define PARCEL_LEN (HEADER_LEN + PARCEL_BODY_LEN + TAIL_LEN)

//---------------------------------------------
//  Protocol - Sync word
#define HEADER_SYNC (0xFF)
#define TAIL_SYNC (0xFE)
#define VALID_VAL_MAX (0xEF)
#define VALID_MIN_THRESHOLD   10

#define BOARD_ID 1  // UART MASTER : 0, UART SLAVE : 1

byte trimVal8(byte raw_value) {
    byte value = 0;

    if(raw_value <= 10)
      raw_value = 0;
    else
      raw_value -= 10;


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

void buildPacket(byte *packet_buffer, int adc_mat_buf[MUX_LIST_LEN][NUM_MUX_OUT], int width, int height) {
    packet_buffer[0] = HEADER_SYNC;       // 0xFF
    packet_buffer[1] = HEADER_SYNC;       // 0xFF
    packet_buffer[2] = 0x00;              // Major Ver
    packet_buffer[3] = 0x00;              // Minor Ver
    packet_buffer[4] = width;  // Packet body Len, width, height,
    packet_buffer[5] = BOARD_ID;          // Board ID
    packet_buffer[6] = 0x00;              // Reserved 0
    packet_buffer[7] = 0x00;              // Reserved 1

    int pa_index = HEADER_LEN;

    for (int w = 0; w < width; w++) {
        for (int h = 0; h < height; h++) {
            packet_buffer[pa_index++] = trimVal8(adc_mat_buf[w][h]);
        }
    }

    packet_buffer[pa_index++] = 0x00;       //  Reserved 2
    packet_buffer[pa_index++] = TAIL_SYNC;  // 0xFE
}

void buildEncodePacket(byte *packet_buffer, int adc_mat_buf[MUX_LIST_LEN][NUM_MUX_OUT], int width, int height) {
    packet_buffer[0] = HEADER_SYNC;       // 0xFF
    packet_buffer[1] = HEADER_SYNC;       // 0xFF
    packet_buffer[2] = 0x00;              // Major Ver
    packet_buffer[3] = 0x00;              // Minor Ver
    packet_buffer[4] = width;  // Packet body Len, width, height,
    packet_buffer[5] = BOARD_ID;          // Board ID
    packet_buffer[6] = 0xFC;              // Reserved 0
    packet_buffer[7] = 0xFB;              // Reserved 1

    int pa_index = HEADER_LEN;

    int offset = 0;
    for (int w = 0; w < width; w++) {
        for (int h = 0; h < height; h++) {
            ori_buffer[offset] = trimVal8(adc_mat_buf[w][h]);
            offset++;
        }
    }

    memset(enc_buffer, 0, SRC_BUF_SIZE);
    int FRAME_SIZE = (width * height);
    int enc_size = rle_encode(ori_buffer, FRAME_SIZE, enc_buffer, FRAME_SIZE);
    int dec_size = rle_decode(enc_buffer, enc_size, dec_buffer, FRAME_SIZE);

    Serial.printf("ori size = %d, enc size = %d, dec size = %d \n", (width * height), enc_size, dec_size);
    bool is_ok = is_same();
    if (is_ok == true) {
        Serial.println("OK");
    } else {
        Serial.println("FAIL");
    }

    offset = 0;
    for (int w = 0; w < width; w++) {
        for (int h = 0; h < height; h++) {
            packet_buffer[pa_index++] = dec_buffer[offset++];
        }
    }

    // memcpy(packet_buffer + HEADER_LEN, dec_buffer, dec_size);
    // pa_index += dec_size;

    packet_buffer[pa_index++] = 0x00;       //  Reserved 2
    packet_buffer[pa_index++] = TAIL_SYNC;  // 0xFE
}

void buildParcel(byte *packet_buffer, int adc_mat_buf[MUX_LIST_LEN][NUM_MUX_OUT], int width, int height, int start_x, int end_x) {
    packet_buffer[0] = HEADER_SYNC;       // 0xFF
    packet_buffer[1] = HEADER_SYNC;       // 0xFF
    packet_buffer[2] = 0x01;              // Major Ver
    packet_buffer[3] = 0x00;              // Minor Ver
    packet_buffer[4] = width;  // Packet body Len, width, height,
    packet_buffer[5] = BOARD_ID;          // Board ID
    packet_buffer[6] = start_x;              // Reserved 0, start column // FC : 252, cause fedcb 54321
    packet_buffer[7] = end_x;              // Reserved 1, end  column // FB : 251, cause fedcb 54321

    int pa_index = HEADER_LEN;

    for (int w = start_x; w < end_x; w++) {
        for (int h = 0; h < height; h++) {
            packet_buffer[pa_index++] = trimVal8(adc_mat_buf[w][h]);
            // packet_buffer[pa_index++] = h + w;
        }
    }

    packet_buffer[pa_index++] = height;     //  Reserved 2
    packet_buffer[pa_index++] = TAIL_SYNC;  // 0xFE
}
