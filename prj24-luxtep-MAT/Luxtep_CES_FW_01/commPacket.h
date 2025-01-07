#ifndef _COMMPACKET_H_
#define _COMMPACKET_H_
/*
  <Arduino Config>
  BOARD : "ESP32-S3 DEVKITC-1" (NOT 1.1) OR "ADAFRUIT QT py esp32-C3"
  DESC : SRAM 512K, ROM 384K, FLASH 16M, PSRAM 8M
  CONFIG : Flash QIO 80MHz, OPI PSRAM, Partition Minimal SPIFFS, others : apply default setting

  SEE 74HC595 later - https://docs.arduino.cc/tutorials/communication/guide-to-shift-out
*/

#include "configPins-mdll-24-6822.h"
#include "libProtocol.h"
#include "lib_ble_ota.h"
#include "lib_gpio.h"
// #include "lib_rle_util.h"
// #include "lib_rle.h"

//---------------------------------------------
//  Serial config
#define SERIAL_SIZE_TX 4096  // used in Serial.setRxBufferSize()
#define SERIAL_SIZE_RX 4096  // used in Serial.setRxBufferSize()

#define BAUD_RATE0 921600
#define BAUD_RATE1 2000000

//  RS485 GPIO
#define pin485U1DE 19
volatile bool receivingData = false;

//---------------------------------------------
//  Luxtep board
#define NUM_LUXTEP_WIDTH 56
#define NUM_LUXTEP_HEIGHT 34

#define HEADER_LEN 8
#define TAIL_LEN 2

#define TX_PACKET_BODY_LEN (NUM_LUXTEP_WIDTH * NUM_LUXTEP_HEIGHT)
#define TX_PACKET_LEN (HEADER_LEN + TX_PACKET_BODY_LEN + TAIL_LEN)

#define PACKET_BODY_LEN TX_PACKET_BODY_LEN
#define PACKET_LEN (HEADER_LEN + PACKET_BODY_LEN + TAIL_LEN)

#define SUB_PACKET_BODY_LEN NUM_SUB_SENSOR_CH
#define SUB_PACKET_LEN (HEADER_LEN + NUM_SUB_SENSOR_CH + TAIL_LEN)

//---------------------------------------------
//  Protocol - Packet

#define IDX_HEADER_0 0
#define IDX_HEADER_1 1
#define IDX_MAJOR_VER 2
#define IDX_MINOR_VER 3
#define IDX_BODY_ROW 4  // height
#define IDX_BODY_COL 5  // width
#define IDX_BOARD_ID 6  // TOP(UART MASTER) : 0, BOTTOM(UART SLAVE) : 1
#define IDX_RES_1 7

#define IDX_TAIL_0 (PACKET_LEN - 2)
#define IDX_TAIL_1 (PACKET_LEN - 1)

//---------------------------------------------
//  Protocol - Parcel
#define HEADER_SYNC (0xFF)
#define TAIL_SYNC (0xFE)
#define VALID_VAL_MAX (0xEF)

byte packetBufSlave[PACKET_LEN * 5];  // * 5 for overflow
int deliver_count_slave = 0;

#define BOARD_ID_TOP 0
#define BOARD_ID_BOTTOM 1

#define BOARD_ID BOARD_ID_TOP  // TOP(UART MASTER) : 0, BOTTOM(UART SLAVE) : 1

byte txPacketBuf[TX_PACKET_LEN];

byte packetBuf[PACKET_LEN];

//---------------------------------------------
// OSD buffer
int osd_start_x = 0;
int osd_start_y = 0;
int osd_width = 0;
int osd_height = 0;

#define OSD_BUFFER_SIZE (NUM_SUB_WIDTH * (NUM_SUB_HEIGHT + 1))
#define OSD_PACKET_SIZE (PACKET_HEAD_LEN + SUB_HEAD_LEN + OSD_BUFFER_SIZE + TAIL_LEN)
byte packetBufOSD[OSD_PACKET_SIZE];
// byte osd_buf[OSD_BUFFER_SIZE];
byte *osd_buf;

//---------------------------------------------
//  Function Declarations
//---------------------------------------------
// extern int loop_count;
extern int adc_scan_count_main;

void sendPacket0(byte *packet_buffer, int packet_len);
void printPacket(byte *packet_buffer, int packet_len);

//---------------------------------------------
//  Function Definitions
//---------------------------------------------
void IRAM_ATTR onUartRx() {
    receivingData = true;  // 데이터 수신 이벤트 감지
}

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

void setup_Comm() {
    //-----------------------------------------
    //  RS485
    pinMode(pin485U1DE, OUTPUT);
    digitalWrite(pin485U1DE, LOW);
}

//  send data to the PC
void sendPacket0(byte *packet_buffer, int packet_len) {
    Serial.write(packet_buffer, packet_len);
    delayMicroseconds(10);

    // Serial.printf("[%8dms] TX [%d->x], seq:%d>TX \n", millis(), packet_buffer[IDX_BOARD_ID], packet_buffer[IDX_RES_1]);
    // printPacket(packet_buffer, packet_len);
}

//  send data via 485 BUS (Half duplex)
bool sendPacket1(byte *packet_buffer, int packet_len) {
    // check if 485 BUS is idle
    if (Serial1.available()) {
        vTaskDelay(0);
        return false;
    };

    digitalWrite(pin485U1DE, HIGH);  // LOW : RX, HIGH : TX
    delay(1);                        // (us) 100: NG, 800:NG, 1000: OK

    Serial1.write(packet_buffer, packet_len);

    delayMicroseconds(600);         // 100: NG, 300:NG, 400: NG
    digitalWrite(pin485U1DE, LOW);  // LOW : RX, HIGH : TX

    delayMicroseconds(10);

    return true;
    // Serial.printf("[%8dms] TX [%d->x], seq:%d>TX \n", millis(), packet_buffer[IDX_BOARD_ID], packet_buffer[IDX_RES_1]);
    //  log all data.
    // printPacket(packet_buffer, packet_len);
}

void printUart1(char *text) {
    digitalWrite(pin485U1DE, HIGH);  // LOW : RX, HIGH : TX
    delay(1);                        // 100: NG, 200:OK, 400: OK

    Serial1.write(text, 64);

    delay(1);                       // 100: NG, 200:OK, 400: OK
    digitalWrite(pin485U1DE, LOW);  // LOW : RX, HIGH : TX
}

bool is_ignore_this_longclick = false;
bool is_command_thistime = false;
bool is_ota_loop_on = false;

bool checkOTAProc() {
    if (TactButtons[0]->isClickedVeryLong == true) {
        if (is_ignore_this_longclick == false) {
            if (is_ota_loop_on == false) {
                setup_ota();
                is_ota_loop_on = true;
                is_ignore_this_longclick = true;
                Serial.printf("ota on \n");
            } else {  // else : if (is_ota_loop_on == true)
                //  stop ota
                is_ota_loop_on = false;
                is_ignore_this_longclick = true;
                Serial.printf("ota off \n");
            }
        }

    } else {
        is_ignore_this_longclick = false;
    }

    if (is_ota_loop_on == true) {
        Serial.printf("ota is working \n");
        loop_ota();
    }

    return is_ota_loop_on;
}

int rx_osd_count_all = 0;
char Serial1_Log[128];

bool isBookTX1 = false;
int bookTX1_len = 0;

int MAIN_TX1() {
    if (isBookTX1 == false)
        return -1;
    if (true == sendPacket1(packetBufSlave, bookTX1_len)) {
        isBookTX1 = false;
        bookTX1_len = 0;
    }
}

int MAIN_RX0_TX1() {
    while (true) {
        int size_avail = Serial.available();

        if (size_avail < HEADER_LEN) {
            // Serial.println("deliver nothing");
            delayMicroseconds(10);
            return -1;
        }

        int error_code = 0;

        int size_read = Serial.readBytes(packetBufSlave, HEADER_LEN);

        if ((packetBufSlave[0] != 0xFF) || (packetBufSlave[1] != 0xFF)) {
            // printUart1("[ERROR] No header  ");
            Serial.readBytes(packetBufSlave, size_avail - HEADER_LEN);
            return -2;
        }
        // sprintf(Serial1_Log, "[%08d] RX, count = %d", millis(), rx_osd_count_all);
        // printUart1(Serial1_Log);

        //  group id
        switch (packetBufSlave[IDX_GROUP_ID]) {
            case G_LED_COMMAND:
                break;
            default:
                return -3;
        }

        //  read sub header
        size_read = Serial.readBytes(packetBufSlave + HEADER_LEN, SUB_HEAD_LEN);

        int x_size = packetBufSlave[10];
        int y_size = packetBufSlave[11];

        int body_len = x_size * y_size;
        int packet_len = HEADER_LEN + SUB_HEAD_LEN + body_len + TAIL_LEN;

        //  read body and tail
        size_read = Serial.readBytes(packetBufSlave + HEADER_LEN + SUB_HEAD_LEN, body_len + TAIL_LEN);

        // sprintf(Serial1_Log, "RX, w=%d, h=%d tail = %d", x_size, y_size, packetBufSlave[HEADER_LEN + SUB_HEAD_LEN + body_len + TAIL_LEN - 1]);
        // printUart1(Serial1_Log);

        if (packetBufSlave[packet_len - 1] != TAIL_SYNC) {
            // sprintf(Serial1_Log, "[E] RX OSD, tail=%d ", packetBufSlave[packet_len - 1]);
            // printUart1(Serial1_Log);
            return -4;
        }

        //  CORRECT Frame

        // sprintf(Serial1_Log, "[%08d] RX OSD, board=%d[%dth], count = %d /0",
        //         millis(), packetBufSlave[IDX_MSG_ID], packetBufSlave[packet_len - 2], rx_osd_count_all);
        // printUart1(Serial1_Log);

        rx_osd_count_all++;

        if (packetBufSlave[IDX_MSG_ID] == M_BOARD_1) {
            if (Serial1.available() == 0)
                sendPacket1(packetBufSlave, packet_len);
            else {
                isBookTX1 = true;
                bookTX1_len = packet_len;
            }
            return 0;
        }

        //   else board id == 0
        byte *oneOSDPacket = packetBufSlave;

        osd_start_x = oneOSDPacket[8];
        osd_start_y = oneOSDPacket[9];
        osd_width = oneOSDPacket[10];
        osd_height = oneOSDPacket[11];

        // osd_buf
        // memset(osd_buf, 0, OSD_BUFFER_SIZE);

        // osd_start_x = 1;
        // osd_start_y = 3;
        // osd_width = 15;
        // osd_height = 3;

        byte r, g, b;

        int packet_offset = PACKET_HEAD_LEN + SUB_HEAD_LEN;

        for (int y = osd_start_y; y < (osd_start_y + osd_height); y++) {
            for (int x = osd_start_x; x < (osd_start_x + osd_width); x++) {
                int pos = y * SIZE_X + x;
                osd_buf[pos] = oneOSDPacket[packet_offset++];
                // sprintf(Serial1_Log, "(X%d, Y%d) %d ", x, y, osd_buf[pos]);
                // printUart1(Serial1_Log);
            }
        }
    }  // while

    return 1;
}

int MAIN_RX1() {
    int size_avail = Serial1.available();
    if (size_avail == 0) {
        // Serial.println("deliver nothing");
        return -1;
    } else if (size_avail < SUB_PACKET_LEN) {
        // Serial.printf("deliver small (%d) \n", size_avail);
        // int size_read = Serial1.readBytes(packetBufSlave, size_avail);

        // // Serial.printf("~~RX0 read %d, board id[%d] \n", size_read, packetBufSlave[IDX_BOARD_ID]);
        // for (int i = 0; i < size_read; i++) {
        //     Serial.printf("[%d]%d, ", i, packetBufSlave[i]);
        //     if ((size_read + 1) % 40 == 0)
        //         Serial.println("~");
        // }
        // Serial.println("~");

        delay(1);
        return -2;
    }

    // readBytesUntil(0xFE, , ) : It reads as far as just before 0xFE. It doesn't include 0xFE.
    // Serial.printf("[%08d] RX1 go\n", millis());
    int size_read = Serial1.readBytesUntil(TAIL_SYNC, packetBufSlave, size_avail);

    int jitter = 0;
    if (SUB_PACKET_LEN < (size_read + 1)) {
        jitter = (size_read + 1) - SUB_PACKET_LEN;
    }

    if ((packetBufSlave[IDX_HEADER_0 + jitter] == HEADER_SYNC) && (packetBufSlave[IDX_HEADER_1 + jitter] == HEADER_SYNC)) {
        packetBufSlave[IDX_TAIL_1 + jitter] = TAIL_SYNC;  // add 0xFE

        int rx_board_id = packetBufSlave[IDX_BOARD_ID + jitter];
        int rx_seq = packetBufSlave[IDX_RES_1 + jitter];

        // Serial.printf("\nRX [%d<-%d], seq:%d<RX \n", dip_decimal, rx_board_id, rx_seq);
        memcpy(force_buf_1, packetBufSlave + HEADER_LEN, SUB_PACKET_LEN);

        // sendPacket0(packetBufSlave + jitter, SUB_PACKET_LEN);
        // printPacket(packetBufSlave, SUB_PACKET_LEN);

        // Serial.printf("[%08d] RX1 end\n", millis());
    }

    // int size_left = Serial1.available();
    // if (SUB_PACKET_LEN < size_left)
    //     Serial1.readBytes(packetBufSlave, size_left);

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
boolean tx_grant_board3 = false;

int SUB_RX1(int rx_duplicate) {
    if ((dip_decimal < 1) || (7 < dip_decimal)) {
        return -1;
    }
    //  cases for only dip_decimal ranges 1 ~ 7

    int read_sum = 0;
    long time_lap = millis();

    int size_avail = Serial1.available();
    // Serial.printf("avail = %d \n", size_avail);
    //  uart 1 for debugging
    {
        // if (size_avail == 0) {
        //     return -1;
        // }

        // Serial1.readBytes(packetBufOSD, size_avail);
        // packetBufOSD[size_avail] = '\0';
        // Serial.printf("[%08d] {%d:%d bytes} %s\n", millis(), rx_duplicate, size_avail, (char *)packetBufOSD);

        // return -1;
    }

    if (size_avail < HEADER_LEN) {
        if (0 < size_avail) {
            Serial.printf("SUB RX (%d), {0x%02x 0x%02x %02d %02d | %02d %02d %02d %02d} \n", size_avail,
                          packetBufOSD[0], packetBufOSD[1], packetBufOSD[2], packetBufOSD[3],
                          packetBufOSD[4], packetBufOSD[5], packetBufOSD[6], packetBufOSD[7]);
        }
        delayMicroseconds(10);
        return -1;
    }

    int error_code = 0;
    memset(packetBufOSD, 0, OSD_PACKET_SIZE);

    int size_read = Serial1.readBytes(packetBufOSD, HEADER_LEN);
    read_sum += size_read;

    if ((packetBufOSD[0] != 0xFF) || (packetBufOSD[1] != 0xFF)) {
        size_read = Serial1.readBytes(packetBufOSD, size_avail - HEADER_LEN);
        read_sum += size_read;
        return -2;
    }

    //  group id
    switch (packetBufOSD[IDX_GROUP_ID]) {
        case G_LED_COMMAND:
            break;
        default:
            return -3;
    }

    //  read sub header
    size_read = Serial1.readBytes(packetBufOSD + HEADER_LEN, SUB_HEAD_LEN);
    read_sum += size_read;

    int x_size = packetBufOSD[10];
    int y_size = packetBufOSD[11];

    int body_len = x_size * y_size;
    int packet_size = HEADER_LEN + SUB_HEAD_LEN + body_len + TAIL_LEN;

    size_avail = Serial1.available();

    //  read body and tail
    size_read = Serial1.readBytes(packetBufOSD + HEADER_LEN + SUB_HEAD_LEN, body_len + TAIL_LEN);

    read_sum += size_read;

    Serial.printf("[%3d ms]SUB RX (%d/%d), total = %d \n", (millis() - time_lap), size_read, size_avail, read_sum);

    if (packetBufOSD[HEADER_LEN + SUB_HEAD_LEN + body_len + TAIL_LEN - 1] != TAIL_SYNC) return -4;

    //  in case G_LED_COMMAND
    if (packetBufOSD[IDX_MSG_ID] != M_BOARD_1) {
        return -5;
    }

    size_avail = Serial1.available();
    Serial.printf("SUB RX (%d), {0x%02x 0x%02x %02d %02d | %02d %02d %02d %02d}~{%02d 0x%02x} \n", size_avail,
                  packetBufOSD[0], packetBufOSD[1], packetBufOSD[2], packetBufOSD[3],
                  packetBufOSD[4], packetBufOSD[5], packetBufOSD[6], packetBufOSD[7],
                  packetBufOSD[HEADER_LEN + SUB_HEAD_LEN + body_len + TAIL_LEN - 2], packetBufOSD[HEADER_LEN + SUB_HEAD_LEN + body_len + TAIL_LEN - 1]);

    byte *oneOSDPacket = packetBufOSD;

    //  otherwise, board id = 0
    osd_start_x = oneOSDPacket[8];
    osd_start_y = oneOSDPacket[9];
    osd_width = oneOSDPacket[10];
    osd_height = oneOSDPacket[11];

    byte r, g, b;

    int packet_offset = PACKET_HEAD_LEN + SUB_HEAD_LEN;

    for (int y = osd_start_y; y < (osd_start_y + osd_height); y++) {
        for (int x = osd_start_x; x < (osd_start_x + osd_width); x++) {
            int index = y * SIZE_X + x;
            osd_buf[index] = oneOSDPacket[packet_offset++];
            // if (y == osd_start_y)
            //     osd_buf[index] = rgb_24to8(220, 0, 0);
            // else if (y == osd_start_y + 1)
            //     osd_buf[index] = rgb_24to8(0, 252, 0);
            // else
            //     osd_buf[index] = rgb_24to8(105, 252, 0);
        }
    }

    // Serial.printf("[%08d] %d \n", millis(), oneOSDPacket[packet_size - 2]);

    deliver_count_slave++;

    return 0;
}

int SUB_RX1_MultiBoard() {
    int size_avail = Serial1.available();
    if (size_avail == 0) {
        // Serial.println("deliver nothing");
        delay(1);
        return -1;
    } else if (size_avail < SUB_PACKET_LEN) {
        // Serial.printf("deliver small 2 (%d) \n", size_avail);
        int size_read = Serial1.readBytes(packetBufSlave, size_avail);

        // Serial.printf("~~RX1 read %d, board id[%d] \n", size_read, packetBufSlave[IDX_BOARD_ID]);
        // for(int i = 0 ; i < size_read ; i++) {
        //   Serial.printf("[%d]%d, ", i, packetBufSlave[i]);
        // }
        // Serial.println("~");
        packetBufSlave[size_read + 1] = '\0';
        Serial.printf("%s \n", (char *)packetBufSlave);
        delay(1);
        return -1;
    }

    // readBytesUntil(0xFE, , ) : It reads as far as just before 0xFE. It doesn't include 0xFE.
    int size_read = Serial1.readBytesUntil(TAIL_SYNC, packetBufSlave, size_avail);  // it doesn't include 'Until' byte. so, size_read += 1
    int size_left = Serial1.available();

    int jitter = 0;
    if (SUB_PACKET_LEN < (size_read + 1)) {
        jitter = (size_read + 1) - SUB_PACKET_LEN;
    }
    // Serial.printf("Jitter : %d \n", jitter);

    if ((packetBufSlave[IDX_HEADER_0 + jitter] == HEADER_SYNC) && (packetBufSlave[IDX_HEADER_1 + jitter] == HEADER_SYNC)) {
        packetBufSlave[IDX_TAIL_1 + jitter] = TAIL_SYNC;  // add 0xFE

        int rx_board_id = packetBufSlave[IDX_BOARD_ID + jitter];
        int rx_seq = packetBufSlave[IDX_RES_1 + jitter];

        Serial.printf("[%8dms] RX [%d<-%d], seq:%d<RX \n", millis(), dip_decimal, rx_board_id, rx_seq);
        // if (rx_board_id == 1)
        //     tx_grant_board2 = true;
        // else if (rx_board_id == 2)
        //     tx_grant_board1 = true;

        // printPacket(packetBufSlave + jitter, SUB_PACKET_LEN);
    } else {
        Serial.printf("**RX1 read %d, board id[%d] \n", size_read, packetBufSlave[IDX_BOARD_ID]);
        for (int i = 0; i < size_read; i++) {
            Serial.printf("[%d]%d, ", i, packetBufSlave[i]);
        }
        Serial.println("~");
    }

    // size_left = Serial1.available();
    // if (SUB_PACKET_LEN < size_left)
    //   Serial1.readBytes(packetBufSlave, size_left);

    // for (int i = 0; i < 50; i++) {
    //   // tempDelay(10);
    //   // size_left = Serial1.available();
    //   // Serial1.readBytes(packetBufSlave, size_left);
    // }

    deliver_count_slave++;

    return 0;
}

void tempDelay(int time_len_ms) {
    delay(time_len_ms);
}

int build_count = 0;

void buildPacket_Luxtep(byte *tx_packet_buffer, int adc_mat_buf[SIZE_X][SIZE_Y], int width, int height) {  // height = 16
    tx_packet_buffer[0] = HEADER_SYNC;                                                                     // 0xFF
    tx_packet_buffer[1] = HEADER_SYNC;                                                                     // 0xFF
    tx_packet_buffer[2] = 0x01;                                                                            // Major Ver
    tx_packet_buffer[3] = 0x00;                                                                            // Minor Ver
    tx_packet_buffer[4] = NUM_LUXTEP_HEIGHT;                                                               // row = height = 12
    tx_packet_buffer[5] = NUM_LUXTEP_WIDTH;                                                                // column = width = 10
    tx_packet_buffer[6] = 0;                                                                               // board id
    // tx_packet_buffer[7] = 0x00;              // Reserved 1
    tx_packet_buffer[7] = build_count % 0xEF;  // Reserved 1

    int pa_index = HEADER_LEN;

    int adc_offset_main = 0;
    int adc_offset_sub = 0;
    for (int y = 0; y < NUM_LUXTEP_HEIGHT; y++) {
        for (int x = 0; x < NUM_LUXTEP_WIDTH; x++) {
            int luxtep_index = y * NUM_LUXTEP_WIDTH + x;

            if ((luxtep_index % NUM_LUXTEP_WIDTH) < SIZE_X) {
                tx_packet_buffer[pa_index] = force_buf_1[adc_offset_sub++];
            } else {
                tx_packet_buffer[pa_index] = force_buf[adc_offset_main++];
                // tx_packet_buffer[pa_index] = 100+(adc_offset_main++)/10;
            }

            if (tx_packet_buffer[pa_index] < 5)
                tx_packet_buffer[pa_index] = 0;

            pa_index++;
        }
    }

    tx_packet_buffer[pa_index++] = 0;          //  Reserved 2
    tx_packet_buffer[pa_index++] = TAIL_SYNC;  // 0xFE

    build_count++;
}

void buildPacket_Luxtep_Sub(byte *tx_packet_buffer, int adc_mat_buf[SIZE_X][SIZE_Y], int width, int height) {  // height = 16
    tx_packet_buffer[0] = HEADER_SYNC;                                                                         // 0xFF
    tx_packet_buffer[1] = HEADER_SYNC;                                                                         // 0xFF
    tx_packet_buffer[2] = 0x01;                                                                                // Major Ver
    tx_packet_buffer[3] = 0x00;                                                                                // Minor Ver
    tx_packet_buffer[4] = height;                                                                              // row = height = 12
    tx_packet_buffer[5] = width;                                                                               // column = width = 10
    tx_packet_buffer[6] = dip_decimal;                                                                         //  board id                                                                     // board id
    // tx_packet_buffer[7] = 0x00;              // Reserved 1
    tx_packet_buffer[7] = build_count % 0xEF;  // Reserved 1

    int pa_index = HEADER_LEN;

    int adc_offset_main = 0;
    int adc_offset_sub = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int luxtep_index = y * width + x;

            tx_packet_buffer[pa_index] = force_buf[adc_offset_main++];
            // tx_packet_buffer[pa_index] = 100+(adc_offset_main++)/10;

            if (tx_packet_buffer[pa_index] < 5)
                tx_packet_buffer[pa_index] = 0;

            pa_index++;
        }
    }

    pa_index = HEADER_LEN + SUB_PACKET_BODY_LEN;

    tx_packet_buffer[pa_index++] = 0;          //  Reserved 2
    tx_packet_buffer[pa_index++] = TAIL_SYNC;  // 0xFE

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

    int height = packet_buffer[4];  // height
    int width = packet_buffer[5];   // width
    int body_len = width * height;

    Serial.printf(" adc data length : %d (w:%d, h:%d)\n", body_len, width, height);

    for (int i = 0; i < body_len; i++) {
        Serial.printf("%3d,", packet_buffer[HEADER_LEN + i]);
        if (((i + 1) % width) == 0)
            Serial.println("~");
    }

    offset += body_len;

    Serial.printf("[%d] %3d ", offset, packet_buffer[offset]);
    offset++;
    Serial.printf("[%d] %3d  \n", offset, packet_buffer[offset]);
    offset++;
}

#endif  // _COMMPACKET_H_