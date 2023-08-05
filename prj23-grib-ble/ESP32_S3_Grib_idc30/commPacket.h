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
//  Protocol - Parcel
#define HEADER_SYNC (0xFF)
#define TAIL_SYNC (0xFE)
#define VALID_VAL_MAX (0xEF)

#define BOARD_ID 0  // UART MASTER : 0, UART SLAVE : 1

byte trimVal8(byte raw_value) {
    byte value = 0;
    if (255 == raw_value) {
        value = VALID_VAL_MAX;
    } else if ((VALID_VAL_MAX - 1) <= raw_value) {
        value = VALID_VAL_MAX - 1;
    } else if (raw_value < 4) {
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
    packet_buffer[4] = (width * height);  // Packet body Len, width, height,
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
    packet_buffer[4] = (width * height);  // Packet body Len, width, height,
    packet_buffer[5] = BOARD_ID;          // Board ID
    packet_buffer[6] = 0x00;              // Reserved 0
    packet_buffer[7] = 0x00;              // Reserved 1

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

byte packetBufSlave[PACKET_LEN * 5];  // * 5 for overflow

int deliver_count_slave = 0;

int deliverSlaveUart() {
    int size_avail = Serial1.available();
    if (size_avail == 0) {
        // Serial.println("deliver nothing");
        return -1;
    } else if (size_avail < PARCEL_LEN) {
        // Serial.printf("deliver small %d \n", size_avail);
        return -1;
    }

    int size_read = Serial1.readBytesUntil(TAIL_SYNC, packetBufSlave, size_avail);
    int size_left = Serial1.available();

/*
    if( (packetBufSlave[0] == HEADER_SYNC) && (packetBufSlave[1] == HEADER_SYNC) ) {
      Serial1.readBytes(&packetBufSlave[size_read], 1);

      Serial.printf("\nloop=%d, avail=%d, read=%d, left=%d, %d} \n", deliver_count_slave,
                size_avail, size_read, size_left, PARCEL_LEN);
      Serial.printf("header8 [%d, %d, %d, %d : %d, %d, %d, %d]\n",
                packetBufSlave[0], packetBufSlave[1],
                packetBufSlave[2], packetBufSlave[3],
                packetBufSlave[4], packetBufSlave[5],
                packetBufSlave[6], packetBufSlave[7]);
      Serial.printf("tail2 [%d, %d, %d, %d]\n"  ,
                  packetBufSlave[PARCEL_LEN -4], packetBufSlave[PARCEL_LEN -3],
                  packetBufSlave[PARCEL_LEN -2], packetBufSlave[PARCEL_LEN -1]);

      for(int mux_id = (MUX_LIST_LEN - 1) ; 0 <= mux_id ; mux_id--) {
        Serial.printf("[mux:%2d] ", mux_id);

        //for (int i = 0 ; i < NUM_MUX_OUT ; i++) {
        for (int i = 0 ; i < NUM_MUX_OUT ; i++) {
          Serial.printf("%3d, ", packetBufSlave[HEADER_LEN + mux_id * NUM_MUX_OUT + i]);
        }
        Serial.println("~");
      }
    }
*/


    if ((packetBufSlave[0] == HEADER_SYNC) && (packetBufSlave[1] == HEADER_SYNC)) {
        packetBufSlave[PARCEL_LEN - 1] = TAIL_SYNC;
        sendSerial(packetBufSlave, (HEADER_LEN + packetBufSlave[4] + TAIL_LEN));
        // Serial.write(packetBufSlave, (HEADER_LEN + packetBufSlave[4] + TAIL_LEN));
    }

    size_left = Serial1.available();
    if(PARCEL_LEN * 8 < size_left)
      Serial1.readBytes(packetBufSlave, size_left);

    deliver_count_slave++;

    return 0;
}
