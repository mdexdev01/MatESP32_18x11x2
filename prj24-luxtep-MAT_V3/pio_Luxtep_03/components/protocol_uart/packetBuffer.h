#ifndef _PACKET_BUFFER_H_
#define _PACKET_BUFFER_H_

#include "libProtocol.h"

//---------------------------------------------
//	VARIABLE DEFINITION
//---------------------------------------------

byte packetBuf[PACKET_LEN_SEN_1SET];
byte packetBuf_DeviceIO[HEAD_LEN + TAIL_LEN];
byte packetBuf_SensorSub[PACKET_LEN_SEN_1Bd * 2];  // * 2 for overflow

byte packetBuf_RX1[PACKET_LEN_SEN_1Bd * 2];  // * 2 for overflow

byte packetHead[(HEAD_LEN + TAIL_LEN) * 2];  // * 2 for overflow

byte packetBuf_Debug[PACKET_LEN_SEN_1SET];

#define OS_1ms (1 / portTICK_PERIOD_MS)
#define OS_10ms (10 / portTICK_PERIOD_MS)
#define OS_100ms (100 / portTICK_PERIOD_MS)
#define OS_500ms (500 / portTICK_PERIOD_MS)

//---------------------------------------------
// OSD buffer
//---------------------------------------------
typedef struct RECT {
    int start_x = 0;
    int start_y = 0;
    int width = 0;
    int height = 0;
} RECT_tag;

#define NUM_OSD_BUF 4
RECT osdRect[NUM_OSD_BUF];

#define NUM_BOARD 8
bool bookOSD_toSend[NUM_BOARD][NUM_OSD_BUF];

byte packetBuf_OSD[PACKET_LEN_OSD];
byte packetBuf_OSDSub[PACKET_LEN_OSD];
bool isSubOSD_Filled = false;

int packetOSD_SizeSub = 0;

int OSD_START_X = 0;
int OSD_START_Y = 0;
int OSD_WIDTH = 0;
int OSD_HEIGHT = 0;

//---------------------------------------------
//	FUNCTION DECLARATION
void printPacketHeader(byte *rx_head, int identify);
void printPacketOSDSubHeader(byte *rx_subhead);
void printPacket(byte *packet_buffer, int packet_len);

//---------------------------------------------
//	COLOR TABLE CONVERSION
//  Web Safety Color 216
//  https://edyoon.tistory.com/1069
//  int rgb_8bit = (r / 51) * 36 + (g / 51) * 6 + b / 51;
//---------------------------------------------
void rgb_8to24(byte rgb_8bit, byte &r, byte &g, byte &b) {
    r = (rgb_8bit / 36) * 51;
    int r_reminder = rgb_8bit % 36;

    g = (r_reminder / 6) * 51;
    b = (r_reminder % 6) * 51;
}

byte rgb_24to8(int r, int g, int b) {
    int rgb_8bit = (r / 51) * 36 + (g / 51) * 6 + b / 51;
    return rgb_8bit;
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

//---------------------------------------------
//	SERIAL FUNCTION DEFINITION
//---------------------------------------------

QueueHandle_t UART0_EventQueue;
QueueHandle_t UART1_EventQueue;

//---------------------------------------------
//	PACKET FUNCTION DEFINITION
//---------------------------------------------
void setup_PacketBuffer(int board_id) {
    //	ALLOCATE BUFFER FROM PSRAM
    memset(packetHead, 0, (HEAD_LEN + TAIL_LEN) * 2);

    memset(packetBuf, 0, PACKET_LEN_SEN_1SET);
    memset(packetBuf_DeviceIO, 0, HEAD_LEN + TAIL_LEN);

    memset(packetBuf_SensorSub, 0, PACKET_LEN_SEN_1Bd * 2);

    memset(packetBuf_OSD, 0, PACKET_LEN_OSD);
    memset(packetBuf_OSDSub, 0, PACKET_LEN_OSD);
}

bool checkPacketHead(byte *head_buf) {
    if ((head_buf[0] == HEADER_SYNC) && (head_buf[1] == HEADER_SYNC))
        return true;

    return false;
}

//---------------------------------------------
//
//---------------------------------------------
void printBuf(byte *buf, int buf_len, int identify) {
    int line_size = 16;
    int lines_num = buf_len / line_size;
    int tail_num = buf_len % line_size;

    int index_in_line = 0;

    uart0_printf("{iden:%d}, len:%d\n", identify, buf_len);

    for (int k = 0; k < buf_len; k++) {
        uart0_printf("%03d,", buf[k]);

        if (index_in_line % 4 == (4 - 1))
            uart0_printf(" : ");

        index_in_line++;

        if (k % line_size == (line_size - 1)) {
            uart0_printf("\n");
            index_in_line = 0;
        }
    }
    uart0_printf("\n");

    /*
      for(int j = 0 ; j < lines_num ; j++) {
        uart0_printf("{iden:%d} data: %3d, %3d, %3d, %3d : %3d, %3d, %3d, %3d : %3d, %3d, %3d, %3d : %3d\n", identify,
                      buf[j+0], buf[j+1], buf[j+2], buf[j+3], buf[j+4], buf[j+5], buf[j+6], buf[j+7],
                      buf[j+8], buf[j+9], buf[j+10], buf[j+11], buf[j+12]);//, buf[j+13]);
      }

      uart0_printf("{iden:%d} data: ", identify);

      int tail_index = lines_num * line_size;

      for(int j = 0 ; j < tail_num ; j++) {
        uart0_printf("%3d, ", buf[tail_index + j]);
      }
    */
    // uart0_printf("{iden:%d}\t data : %s  \n", identify, (char *)buf);
}

void printPacketHeader(byte *rx_head, int identify) {
    uart0_printf("[%8d]\t {%d}Header  %d, %d, %d, %d : %d, %d, %d, %d \n", millis(), identify,
                 rx_head[0], rx_head[1], rx_head[2], rx_head[3],
                 rx_head[4], rx_head[5], rx_head[6], rx_head[7]);
}

void printPacketOSDSubHeader(byte *rx_subhead) {
    uart0_printf("[%8d]\t Sub Header :%d, %d, %d, %d, %d, %d, %d, %d \n",
                 rx_subhead[0], rx_subhead[1], rx_subhead[2], rx_subhead[3],
                 rx_subhead[4], rx_subhead[5], rx_subhead[6], rx_subhead[7]);
}

extern int adc_scan_count_main;

void printPacket(byte *packet_buffer, int packet_len) {
    //  log all data.
    int offset = 0;
    // uart0_printf("loop=%d [s=%d, l=%d]\n", adc_scan_count_main, packet_buffer[offset], packet_buffer[packet_len - 1]);
    uart0_printf("loop [s=%d, l=%d]\n", packet_buffer[offset], packet_buffer[packet_len - 1]);
    uart0_printf("[%d] %3d ", offset, packet_buffer[offset]);
    offset++;
    uart0_printf("[%d] %3d ", offset, packet_buffer[offset]);
    offset++;
    uart0_printf("[%d] %3d ", offset, packet_buffer[offset]);
    offset++;
    uart0_printf("[%d] %3d ", offset, packet_buffer[offset]);
    offset++;
    uart0_printf("[%d] %3d ", offset, packet_buffer[offset]);
    offset++;
    uart0_printf("[%d] %3d ", offset, packet_buffer[offset]);
    offset++;
    uart0_printf("[%d] %3d ", offset, packet_buffer[offset]);
    offset++;
    uart0_printf("[%d] %3d  \n", offset, packet_buffer[offset]);
    offset++;

    int height = packet_buffer[4];  // height
    int width = packet_buffer[5];   // width
    int body_len = width * height;

    uart0_printf(" adc data length : %d (w:%d, h:%d)\n", body_len, width, height);

    for (int i = 0; i < body_len; i++) {
        uart0_printf("%3d,", packet_buffer[HEAD_LEN + i]);
        if (((i + 1) % width) == 0)
            uart0_printf("~ \n");
    }

    offset += body_len;

    uart0_printf("[%d] %3d ", offset, packet_buffer[offset]);
    offset++;
    uart0_printf("[%d] %3d  \n", offset, packet_buffer[offset]);
    offset++;
}

#endif  // _PACKET_BUFFER_H_