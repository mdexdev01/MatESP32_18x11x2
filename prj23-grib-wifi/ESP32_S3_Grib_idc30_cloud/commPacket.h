/*
  <Arduino Config>
  BOARD : "ESP32-S3 DEVKITC-1" (NOT 1.1) OR "ADAFRUIT QT py esp32-C3"
  DESC : SRAM 512K, ROM 384K, FLASH 16M, PSRAM 8M  
  CONFIG : Flash QIO 80MHz, OPI PSRAM, Partition Minimal SPIFFS, others : apply default setting

  SEE 74HC595 later - https://docs.arduino.cc/tutorials/communication/guide-to-shift-out
*/
#include "configPins.h"

//---------------------------------------------
//  Serial config
#define SERIAL_SIZE_TX  512    // used in Serial.setRxBufferSize()
#define SERIAL_SIZE_RX  1024    // used in Serial.setRxBufferSize()

#define BAUD_RATE   230400

//---------------------------------------------
//  Protocol - Packet
#define HEADER_LEN  8
#define TAIL_LEN    2

#define PACKET_BODY_LEN   (NUM_MUX_OUT * MUX_LIST_LEN)
#define PACKET_LEN        (HEADER_LEN + PACKET_BODY_LEN + TAIL_LEN)

byte packetBuf[PACKET_LEN];

//---------------------------------------------
//  Protocol - Parcel
#define PARCEL_MUX_LEN    6
// #define PACKET_DIV        (MUX_LIST_LEN / PARCEL_MUX_LEN) //  18 / 6
#define PARCEL_BODY_LEN   (NUM_MUX_OUT * PARCEL_MUX_LEN)
#define PARCEL_LEN        (HEADER_LEN + PARCEL_BODY_LEN + TAIL_LEN)

//---------------------------------------------
//  Protocol - Parcel
#define HEADER_SYNC   (0xFF)
#define TAIL_SYNC     (0xFE)
#define VALID_VAL_MAX (0xEF)

#define BOARD_ID    0 // UART MASTER : 0, UART SLAVE : 1


byte trimVal8(byte raw_value) {
  byte value = 0;
  if (255 == raw_value) {
    value = VALID_VAL_MAX;
  }
  else if ( (VALID_VAL_MAX - 1) <= raw_value) {
    value = VALID_VAL_MAX - 1;
  }
  else if (raw_value < 4) {
    value = 0;
  }
  else {
    value = raw_value;
  }

  return value;
}


void buildPacket(byte * packet_buffer, int adc_mat_buf[MUX_LIST_LEN][NUM_MUX_OUT], int width, int height) {
  packet_buffer[0]  = HEADER_SYNC; // 0xFF
  packet_buffer[1]  = HEADER_SYNC; // 0xFF
  packet_buffer[2]  = 0x00; // Major Ver
  packet_buffer[3]  = 0x00; // Minor Ver
  packet_buffer[4]  = (width * height); // Packet body Len, width, height, 
  packet_buffer[5]  = BOARD_ID; // Board ID
  packet_buffer[6]  = 0x00; // Reserved 0
  packet_buffer[7]  = 0x00; // Reserved 1

  int pa_index = HEADER_LEN;

  for (int w = 0 ; w < width ; w++) {
  
    for (int h = 0 ; h < height ; h++) {

      packet_buffer[pa_index++] = trimVal8(adc_mat_buf[w][h]);
    }
  }
  
  packet_buffer[pa_index++] = 0x00; //  Reserved 2
  packet_buffer[pa_index++] = TAIL_SYNC; // 0xFE

}


byte packetBufSlave[PACKET_LEN * 5]; // * 5 for overflow

int deliver_count = 0;

int deliverSlaveUart() {
  int buf_len = Serial1.available();  
  if(buf_len == 0) {
    // Serial.println("deliver nothing");
    return -1;
  }
  else if (buf_len < PARCEL_LEN) {
    // Serial.printf("deliver small %d \n", buf_len);
    return -1;
  }

  int read_num = Serial1.readBytesUntil(TAIL_SYNC, packetBufSlave, buf_len);
  int buf_len2 = Serial1.available();

/*
  if( (packetBufSlave[0] == HEADER_SYNC) && (packetBufSlave[1] == HEADER_SYNC) ) {
    Serial.printf("\nloop=%d, len={%d, %d, %d, %d} [%d, %d, %d, %d, %d, %d, {%d, %d}]\n", deliver_count, 
              buf_len, read_num, buf_len2, PARCEL_LEN,
              packetBufSlave[0], packetBufSlave[1], 
              packetBufSlave[2], packetBufSlave[3],
              packetBufSlave[4], packetBufSlave[5],
              packetBufSlave[6], packetBufSlave[7]);
    Serial.printf("tail (%d) [%d, %d, %d, %d]\n"  , 
                PARCEL_LEN -8,
                packetBufSlave[PARCEL_LEN -4], packetBufSlave[PARCEL_LEN -3],
                packetBufSlave[PARCEL_LEN -2], packetBufSlave[PARCEL_LEN -1]);

    // for(int mux_id = (MUX_LIST_LEN - 1) ; 0 <= mux_id ; mux_id--) {
    for(int mux_id = (6 - 1) ; 0 <= mux_id ; mux_id--) {
      Serial.printf("[mux:%2d] ", mux_id + packetBufSlave[6]);

      //for (int i = 0 ; i < NUM_MUX_OUT ; i++) {
      for (int i = 0 ; i < NUM_MUX_OUT ; i++) {
        Serial.printf("%3d, ", packetBufSlave[HEADER_LEN + mux_id * NUM_MUX_OUT + i]);
      }
      Serial.println("~");
    }
  }
*/
  if( (packetBufSlave[0] == HEADER_SYNC) && (packetBufSlave[1] == HEADER_SYNC) ) {
    packetBufSlave[PARCEL_LEN - 1] = TAIL_SYNC;
    Serial.write(packetBufSlave, (HEADER_LEN + packetBufSlave[4] + TAIL_LEN));
  }

  // if(PARCEL_LEN * 8 < buf_len2)
  //   Serial1.readBytes(packetBufSlave, buf_len2);

  deliver_count++;

  return 0;

}


