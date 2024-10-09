/*
  <Arduino Config>
  BOARD : "ESP32-S3 DEVKITC-1" (NOT 1.1) OR "ADAFRUIT QT py esp32-C3"
  DESC : SRAM 512K, ROM 384K, FLASH 16M, PSRAM 8M
  CONFIG : Flash QIO 80MHz, OPI PSRAM, Partition Minimal SPIFFS, others : apply default setting

  SEE 74HC595 later - https://docs.arduino.cc/tutorials/communication/guide-to-shift-out
*/
#include "configPins.h"
// #include "lib_rle_util.h"
// #include "lib_rle.h"

//---------------------------------------------
//  Serial config
#define SERIAL_SIZE_TX 512   // used in Serial.setRxBufferSize()
#define SERIAL_SIZE_RX 1024  // used in Serial.setRxBufferSize()

// #define BAUD_RATE0 230400
#define BAUD_RATE0 2000000
#define BAUD_RATE1 1000000

//---------------------------------------------
//  Protocol - Packet
#define HEADER_LEN 8
#define TAIL_LEN 2

#define PACKET_BODY_LEN NUM_REAL_SENSOR_CH
#define PACKET_LEN (HEADER_LEN + PACKET_BODY_LEN + TAIL_LEN)

byte packetBuf[PACKET_LEN];

#define IDX_HEADER_0 0
#define IDX_HEADER_1 1
#define IDX_MAJOR_VER 2
#define IDX_MINOR_VER 3
#define IDX_BODY_ROW 4
#define IDX_BODY_COL 5
#define IDX_BOARD_ID 6  // TOP(UART MASTER) : 0, BOTTOM(UART SLAVE) : 1
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
extern int loop_count;
extern int adc_scan_count_main;

void sendPacket0(byte *packet_buffer, int packet_len);
void printPacket(byte *packet_buffer, int packet_len);




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


int MainBoard_Rx1() {
  int size_avail = Serial1.available();
  if (size_avail == 0) {
    // Serial.println("deliver nothing");
    delay(4);
    return -1;
  } else if (size_avail < PACKET_LEN) {
    // Serial.printf("deliver small (%d) \n", size_avail);
    delay(2);
    return -1;
  }


  // readBytesUntil(0xFE, , ) : It reads as far as just before 0xFE. It doesn't include 0xFE.
  int size_read = Serial1.readBytesUntil(TAIL_SYNC, packetBufSlave, size_avail);
  int size_left = Serial1.available();

  int jitter = 0;
  if (PACKET_LEN < (size_read + 1)) {
    jitter = (size_read + 1) - PACKET_LEN;
  }

  if ((packetBufSlave[IDX_HEADER_0 + jitter] == HEADER_SYNC) && (packetBufSlave[IDX_HEADER_1 + jitter] == HEADER_SYNC)) {
    packetBufSlave[IDX_TAIL_1 + jitter] = TAIL_SYNC;  // add 0xFE

    int rx_board_id = packetBufSlave[IDX_BOARD_ID + jitter];
    int rx_seq = packetBufSlave[IDX_RES_1 + jitter];

    // Serial.printf("RX [%d<-%d], seq:%d<RX \n", dip_decimal, rx_board_id, rx_seq);

    sendPacket0(packetBufSlave + jitter, PACKET_LEN);
    // printPacket(packetBufSlave, PACKET_LEN);
  }

  size_left = Serial1.available();
  if (PACKET_LEN < size_left)
    Serial1.readBytes(packetBufSlave, size_left);

  for (int i = 0; i < 50; i++) {
    // tempDelay(10);
    // size_left = Serial1.available();
    // Serial1.readBytes(packetBufSlave, size_left);
  }

  deliver_count_slave++;

  return 0;
}

boolean tx_grant_board1 = true;
boolean tx_grant_board2 = false;

int SubBoard_Rx1() {

  int size_avail = Serial1.available();
  if (size_avail == 0) {
    // Serial.println("deliver nothing");
    delay(4);
    return -1;
  } else if (size_avail < PACKET_LEN) {
    Serial.printf("deliver small 2 (%d) \n", size_avail);
    // int size_read = Serial1.readBytes(packetBufSlave, size_avail);

    // Serial.printf("~~RX1 read %d, board id[%d] \n", size_read, packetBufSlave[IDX_BOARD_ID]);
    // for(int i = 0 ; i < size_read ; i++) {
    //   Serial.printf("[%d]%d, ", i, packetBufSlave[i]);
    // }
    // Serial.println("~");
    delay(2);
    return -1;
  }

  // readBytesUntil(0xFE, , ) : It reads as far as just before 0xFE. It doesn't include 0xFE.
  int size_read = Serial1.readBytesUntil(TAIL_SYNC, packetBufSlave, size_avail);  // it doesn't include 'Until' byte. so, size_read += 1
  int size_left = Serial1.available();

  int jitter = 0;
  if (PACKET_LEN < (size_read + 1)) {
    jitter = (size_read + 1) - PACKET_LEN;
  }
  // Serial.printf("Jitter : %d \n", jitter);


  if ((packetBufSlave[IDX_HEADER_0 + jitter] == HEADER_SYNC) && (packetBufSlave[IDX_HEADER_1 + jitter] == HEADER_SYNC)) {
    packetBufSlave[IDX_TAIL_1 + jitter] = TAIL_SYNC;  // add 0xFE

    int rx_board_id = packetBufSlave[IDX_BOARD_ID + jitter];
    int rx_seq = packetBufSlave[IDX_RES_1 + jitter];

    Serial.printf("[%8dms] RX [%d<-%d], seq:%d<RX \n", millis(), dip_decimal, rx_board_id, rx_seq);
    if (rx_board_id == 1)
      tx_grant_board2 = true;
    else if (rx_board_id == 2)
      tx_grant_board1 = true;

    // printPacket(packetBufSlave + jitter, PACKET_LEN);
  } else {
    Serial.printf("**RX1 read %d, board id[%d] \n", size_read, packetBufSlave[IDX_BOARD_ID]);
    for (int i = 0; i < size_read; i++) {
      Serial.printf("[%d]%d, ", i, packetBufSlave[i]);
    }
    Serial.println("~");
  }

  size_left = Serial1.available();
  if (PACKET_LEN < size_left)
    Serial1.readBytes(packetBufSlave, size_left);

  for (int i = 0; i < 50; i++) {
    // tempDelay(10);
    // size_left = Serial1.available();
    // Serial1.readBytes(packetBufSlave, size_left);
  }

  deliver_count_slave++;

  return 0;
}



void tempDelay(int time_len_ms) {
  delay(time_len_ms);
}

int build_count = 0;
void buildPacket_brandContents(byte *packet_buffer, int adc_mat_buf[MUX_LIST_LEN][NUM_MUX_OUT], int width, int height) {  // height = 16
  packet_buffer[0] = HEADER_SYNC;                                                                                         // 0xFF
  packet_buffer[1] = HEADER_SYNC;                                                                                         // 0xFF
  packet_buffer[2] = 0x01;                                                                                                // Major Ver
  packet_buffer[3] = 0x00;                                                                                                // Minor Ver
  packet_buffer[4] = NUM_REAL_HEIGHT;                                                                                     // row = height = 12
  packet_buffer[5] = NUM_REAL_WIDTH;                                                                                      // column = width = 10
  packet_buffer[6] = dip_decimal;                                                                                         // board id
  // packet_buffer[7] = 0x00;              // Reserved 1
  packet_buffer[7] = build_count % 0xEF;  // Reserved 1

  int pa_index = HEADER_LEN;

  for (int mux = 0; mux < 8; mux++) {
    for (int y_id = 0; y_id < 16; y_id++) {
      int counter = mux * 16 + y_id;
      // packet_buffer[pa_index++] = adc_mat_buf[mux][y_id];
      if (adc_ordered[counter] < 5)
        packet_buffer[pa_index++] = 0;
      else
        packet_buffer[pa_index++] = adc_ordered[counter];

      if (counter == NUM_REAL_SENSOR_CH)
        break;
    }
  }


  pa_index = HEADER_LEN + NUM_REAL_SENSOR_CH;

  packet_buffer[pa_index++] = 0;          //  Reserved 2
  packet_buffer[pa_index++] = TAIL_SYNC;  // 0xFE

  build_count++;
}



void printHexa(byte *hex_data, int hex_len) {
  for (int i = 0; i < hex_len; i++) {
    Serial.printf("%02x", hex_data[i]);
  }
}

void printPacket(byte *packet_buffer, int packet_len) {
  //  log all data.
  int offset = 0;
  Serial.printf("loop=%d [s=%d, l=%d]\n", adc_scan_count_main, packet_buffer[offset], packet_buffer[packet_len - 1]);
  Serial.printf("[%d] %3d ", offset, packet_buffer[offset]);
  offset++;
  Serial.printf("[%d] %3d ", offset, packet_buffer[offset]);
  offset++;
  Serial.printf("[%d] %3d ", offset, packet_buffer[offset]);
  offset++;
  Serial.printf("[%d] %3d ", offset, packet_buffer[offset]);
  offset++;
  Serial.printf("[%d] %3d ", offset, packet_buffer[offset]);
  offset++;
  Serial.printf("[%d] %3d ", offset, packet_buffer[offset]);
  offset++;
  Serial.printf("[%d] %3d ", offset, packet_buffer[offset]);
  offset++;
  Serial.printf("[%d] %3d  \n", offset, packet_buffer[offset]);
  offset++;

  int matrix_num_unit = packet_buffer[4];
  int matrix_unit_len = packet_buffer[5];
  int body_len = matrix_num_unit * matrix_unit_len;

  Serial.printf(" adc data length : %d \n", body_len);

  for (int i = 0; i < body_len; i++) {
    Serial.printf("%3d,", packet_buffer[HEADER_LEN + i]);
    if (((i + 1) % matrix_num_unit) == 0)
      Serial.println("~");
  }

  offset += body_len;

  Serial.printf("[%d] %3d ", offset, packet_buffer[offset]);
  offset++;
  Serial.printf("[%d] %3d  \n", offset, packet_buffer[offset]);
  offset++;
}
