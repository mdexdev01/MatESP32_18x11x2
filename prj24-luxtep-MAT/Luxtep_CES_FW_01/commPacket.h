#ifndef _COMMPACKET_H_
#define _COMMPACKET_H_
/*
  <Arduino Config>
  BOARD : "ESP32-S3 DEVKITC-1" (NOT 1.1) OR "ADAFRUIT QT py esp32-C3"
  DESC : SRAM 512K, ROM 384K, FLASH 16M, PSRAM 8M
  CONFIG : Flash QIO 80MHz, OPI PSRAM, Partition Minimal SPIFFS, others : apply default setting

  SEE 74HC595 later - https://docs.arduino.cc/tutorials/communication/guide-to-shift-out
*/

// #include <Arduino.h>
// #include <stdarg.h>
// #include <stdio.h>

#include "configPins-mdll-24-6822.h"
// #include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "hal/uart_ll.h"
#include "libPacket/groupDeviceIO.h"
#include "libPacket/groupOSDCommand.h"
#include "libPacket/groupSensorData.h"
#include "libPacket/libProtocol.h"
#include "libPacket/packetBuffer.h"
#include "soc/uart_reg.h"
#include "soc/uart_struct.h"
// #include "lib_ble_ota.h"
#include "lib_gpio.h"
// #include "lib_rle_util.h"
// #include "lib_rle.h"

//---------------------------------------------
//  HARDWARE SPEC
//---------------------------------------------
//  RS485 GPIO
#define pin485U1DE 19
#define TX_PIN 17
#define RX_PIN 18

//------------------------------------------------------------
//  Serial config
//------------------------------------------------------------
#define SERIAL_SIZE_TX 2048  // used in Serial.setRxBufferSize()
#define SERIAL_SIZE_RX 2048  // used in Serial.setRxBufferSize()

#define BAUD_RATE0 921600
#define BAUD_RATE1 3000000

//---------------------------------------------
//  Declarations
//---------------------------------------------
extern int adc_scan_count_main;
extern int adc_scan_done;

int pump_count = 0;
int tick_permit_last = 0;

int processRX0(int event_size);
int processRX1(byte *rx1_packet, int rx1_packet_size);
// int processRX1(int rx1_packet_size);
int processRX1_SubDebug(int rx1_packet_size);

bool isSubOSD_Filled = false;

bool rs485Bus_granted = false;

extern int indi_1_r;
extern int indi_1_g;
extern int indi_1_b;

//---------------------------------------------
//  UART0 EVENT HANDLER
//---------------------------------------------

bool processHeader(int event_size);
void triggerDMA_RX(int already_read);
void processDMA_RX1();

void disable_uart_rx_events(int uart_num);
void enable_uart_rx_events(int uart_num);
void clear_uart_event_queue(QueueHandle_t queue);

//---------------------------------------------
//  Function Definitions
//---------------------------------------------
void disable_uart_rx_events(int uart_num) {
    uart_disable_intr_mask(uart_num, UART_RXFIFO_FULL_INT_ENA_M | UART_RXFIFO_TOUT_INT_ENA_M | UART_BRK_DET_INT_ENA_M);
}

void enable_uart_rx_events(int uart_num) {
    uart_enable_intr_mask(uart_num, UART_RXFIFO_FULL_INT_ENA_M | UART_RXFIFO_TOUT_INT_ENA_M | UART_BRK_DET_INT_ENA_M);
}

void clear_uart_event_queue(QueueHandle_t queue) {
    uart_event_t event;
    byte *eventBuffer = (byte *)malloc(128);

    // while (uxQueueMessagesWaiting(queue) > 0) {
    //     xQueueReceive(queue, &event, 0);  // 큐에서 하나씩 제거
    // }

    while (true) {
        int queue_num = uxQueueMessagesWaiting(queue);
        if (queue_num == 0)
            break;

        if (15 < queue_num) {  // 15 : one packet size is 8 or 9, and 15 is 8 * 2 - 1.
            uart0_printf("[%8d] reset RX1 queue with %d \n", millis(), queue_num);
            xQueueReset(UART1_EventQueue);
            uart_flush(UART_NUM_1);
            break;
        }

        xQueueReceive(queue, &event, 0);  // 큐에서 하나씩 제거
        int read_len = uart_read_bytes(UART_NUM_1, eventBuffer, event.size, 5 / portTICK_PERIOD_MS);

        uart0_printf("[%8d]clear queue num = %2d, type= %d, size= %3d : ", millis(), queue_num, event.type, event.size);
        uart0_printf("data = [0]%3d, [1]%3d, ~ [%3d]%3d \n",
                     eventBuffer[0], eventBuffer[1],
                     event.size - 1, eventBuffer[event.size - 1]);

        uart_flush_input(UART_NUM_1);
    }
    free(eventBuffer);
}

void setup_RS485() {
    pinMode(pin485U1DE, OUTPUT);    //  RS485
    digitalWrite(pin485U1DE, LOW);  // LOW : RX, HIGH : TX
}

#include "esp_timer.h"

int64_t wait_tx_done_async(int uart_num, int max_wait_duration_ms) {
    int64_t lap_enter = esp_timer_get_time();  // 마이크로초 단위 시간 측정

    while (true) {
        esp_err_t result = uart_wait_tx_done(uart_num, 0);  // 즉시 반환 모드

        int elapsed_time = esp_timer_get_time() - lap_enter;

        if (result == ESP_OK)
            return elapsed_time;  // ==> 주로 여기에서 반환됨. 3500 ~ 4800us . 반면에 수신은 2800us 정도.

        // 강제 리턴 조건. 타임아웃 체크 (μs → ms 변환)
        if (max_wait_duration_ms * 1000 < elapsed_time) {
            uart0_printf("[%8lldms] TX Timeout\n", esp_timer_get_time() / 1000);
            return elapsed_time;
        }

        vTaskDelay(1);  // 1ms 대기. 1ms 이상 쉼.
    }

    return max_wait_duration_ms * 1000 + 1;
}

//  send data to the PC
void sendPacket0(byte *packet_buffer, int packet_len) {
    int cur_time = millis();
    // uart0_printf("[%8d] TX0, go %dth \n", millis(), pump_count);

    uart_write_bytes(UART_NUM_0, packet_buffer, packet_len);  // 이거 non-blocking으로 바꿔야 함.
    uart0_printf("\n");

    int time_dur_ms = 50;  // 11ms per one board
    int tx_dur_us = wait_tx_done_async(0, time_dur_ms);
    if (time_dur_ms <= (tx_dur_us / 1000))
        uart0_printf("[%8d] TX0, Time Err %dus \n", millis(), tx_dur_us);

    // uart0_printf("[%8d] TX0, size= %d, dur=%d ms, %dth \n", millis(), packet_len, millis() - cur_time, pump_count);
    // printPacket(packet_buffer, packet_len);
}

void setDE(bool state) {
    if (state) {
        // disable_uart_rx_events(UART_NUM_1);  // RX 이벤트 비활성화

        digitalWrite(pin485U1DE, HIGH);  // LOW : RX, HIGH : TX
        delayMicroseconds(650 + 0);      // 안정화 지연
    } else {
        delayMicroseconds(150 + 0);
        digitalWrite(pin485U1DE, LOW);  // LOW : RX, HIGH : TX
        delayMicroseconds(3);

        //   uart_flush_input(UART_NUM_1);
        // enable_uart_rx_events(UART_NUM_1);  // RX 이벤트 다시 활성화
    }
}

//  send data via 485 BUS (Half duplex)
bool sendPacket1(byte *packet_buffer, int packet_len) {
    int64_t cur_snap = esp_timer_get_time();  // 마이크로초 단위로 타이머 초기화
    uart0_printf("[%8d] TX1 go \n", millis());

    int tx_len = 0;

    setDE(true);  // 송신 모드 전환

    tx_len = uart_write_bytes(UART_NUM_1, packet_buffer, packet_len);
    int64_t core_tx_dur_us = wait_tx_done_async(UART_NUM_1, 20);  // uart_num, max_wait_duration_ms

    setDE(false);  // 수신 모드 전환

    int64_t duration_us = esp_timer_get_time() - cur_snap;

    uart0_printf("[%8d] (DUR=%lld in %lld us) TX1 len = %d, { [2]%3d, [7]%3d } \n",
                 millis(), core_tx_dur_us, duration_us,
                 tx_len, packet_buffer[2], packet_buffer[7]);

    return true;
}

void uart0_event_task(void *pvParameters) {
    uart0_printf("UART 0 RX ENTER - EVENT:%d, %d, %d, %d \n", UART_DATA, UART_BREAK, UART_BUFFER_FULL, UART_FIFO_OVF);
    uart_flush_input(UART_NUM_0);

    uart_event_t event;
    while (true) {
        if (xQueueReceive(UART0_EventQueue, (void *)&event, portMAX_DELAY)) {
            // uart0_printf("\n[%d]UART0 Event : %d, size=%d \n", MY_BOARD_ID, event.type, event.size);
            switch (event.type) {
                case UART_DATA:
                    processRX0(event.size);  // 패킷 처리
                    break;

                case UART_BREAK:
                    uart0_println("RX0 BREAK!, len=%d", event.size);
                    uart_flush_input(UART_NUM_0);
                    break;

                case UART_BUFFER_FULL:
                    uart0_println("RX0 Buffer Full!, len=%d", event.size);
                    uart_flush_input(UART_NUM_0);
                    break;

                case UART_FIFO_OVF:
                    uart0_println("RX0 FIFO Overflow!, len=%d", event.size);
                    uart_flush_input(UART_NUM_0);
                    break;

                default:
                    uart0_println("uart 0 default case. event type=%d, len=%d", event.type, event.size);
                    break;
            }
        }

        taskYIELD();
    }
}

//---------------------------------------------
//---------------------------------------------
//   UART1 EVENT HANDLER and Parser/Builder
//---------------------------------------------
//---------------------------------------------

int one_packet_len = 0;
int rx1_events_in_one_packet = 0;
int64_t cur_snap = 0;  // us 단위로 변경

int64_t rx1_snap = 0;
int64_t rx1_work_time = 0;
int64_t rx1_work_total_time = 0;
void uart1_event_task(void *pvParameters) {
    uart0_printf("UART 1 RX ENTER - EVENT:%d, %d, %d, %d \n", UART_DATA, UART_BREAK, UART_BUFFER_FULL, UART_FIFO_OVF);

    byte *currentRxBuffer = packetBuf_RX1;  // temporary
    byte *eventBuffer = (byte *)malloc(128);

    enable_uart_rx_events(UART_NUM_1);  // RX 이벤트 활성화

    uart_event_t event;
    while (true) {
        if (xQueueReceive(UART1_EventQueue, (void *)&event, portMAX_DELAY)) {
            // uart0_printf("\n[%8d]UART1 Event : %d, size=%d \n", millis(), event.type, event.size);

            switch (event.type) {
                case UART_DATA: {
                    rx1_snap = esp_timer_get_time();
                    int read_len = uart_read_bytes(UART_NUM_1, eventBuffer, event.size, 5 / portTICK_PERIOD_MS);
                    rx1_work_time = esp_timer_get_time() - rx1_snap;

                    if (eventBuffer[0] == HEADER_SYNC && eventBuffer[1] == HEADER_SYNC) {
                        // uart0_printf("[%8d] rx1 go [2]%d \n", millis(), eventBuffer[2]);
                        rx1_events_in_one_packet = 0;
                        one_packet_len = 0;
                        cur_snap = esp_timer_get_time();  // 마이크로초 단위로 타이머 초기화

                        rx1_work_total_time = 0;

                        memset(currentRxBuffer, 0, PACKET_LEN_SEN_1Bd);
                    }
                    memcpy(currentRxBuffer + one_packet_len, eventBuffer, event.size);
                    one_packet_len += event.size;
                    rx1_events_in_one_packet++;

                    rx1_work_total_time += rx1_work_time;

                    int queue_num = uxQueueMessagesWaiting(UART1_EventQueue);
                    if (eventBuffer[event.size - 1] == TAIL_SYNC) {
                        // if ((currentRxBuffer[2] % 100 == 0) || (currentRxBuffer[2] % 100 == 1))
                        // uart0_printf("[%8d] rx1 packet, dur=%5lld us(work=%5lld), events: %dea(remain q:%d), size: %d [2]%d\n",
                        //              millis(), esp_timer_get_time() - cur_snap, rx1_work_total_time,
                        //              rx1_events_in_one_packet, queue_num, one_packet_len, currentRxBuffer[2]);

                        processRX1(currentRxBuffer, one_packet_len);  // 패킷 처리
                    } else {
                        if (9 < rx1_events_in_one_packet) {  // 9  is the maximum number of events in one packet.
                            uart0_printf("\n[%8d] rx1 packet WRONG, dur=%5lld us, events: %dea(remain:%d), size: %d WRONG\n",
                                         millis(), esp_timer_get_time() - cur_snap, rx1_events_in_one_packet, queue_num, one_packet_len);
                            // printBuf(currentRxBuffer + one_packet_len - event.size, event.size, rx1_events_in_one_packet);
                            xQueueReset(UART1_EventQueue);
                            uart_flush(UART_NUM_1);
                        }
                    }
                    // uart0_printf("\n[%8d]UART1 UART_DATA done \n", millis());

                } break;
                case UART_BREAK:
                case UART_BUFFER_FULL:
                case UART_FIFO_OVF:
                    uart0_println("[%8d]UART1 Event : %d, size=%d", millis(), event.type, event.size);
                    xQueueReset(UART1_EventQueue);
                    uart_flush(UART_NUM_1);
                    // // RX 이벤트 큐 정리 (불필요한 이벤트 제거)
                    //         clear_uart_event_queue(UART1_EventQueue);
                    break;
                case UART_PATTERN_DET:
                    uart0_println("[%8d]UART1 Event : %d, size=%d", millis(), event.type, event.size);
                    uart_flush_input(UART_NUM_1);
                    break;
                default:
                    break;
            }
        }

        taskYIELD();  // 아무 이벤트가 없으면 CPU 점유권을 넘김
    }
}

char buffer[256];  // 출력 버퍼 크기 (필요에 따라 조정)
void printUart1(const char *fmt, ...) {
    return;
    memset(buffer, 0, 256);

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    setDE(true);  // 송신 모드 전환

    vTaskDelay(1);
    vTaskDelay(1);
    vTaskDelay(1);

    // UART1으로 데이터 송신
    // uart_write_bytes(UART_NUM_1, buffer, strlen(buffer));
    uart_write_bytes(UART_NUM_1, buffer, 256);

    // wait_tx_done_async(1, 150);

    setDE(false);  // 수신 모드 전환
}

int processRX0(int event_size) {
    uart_flush_input(UART_NUM_0);  // RX 버퍼 memset

    byte *rx_head = packetHead;
    byte *currentRxBuffer;
    memset(rx_head, 0, HEAD_LEN);

    // int read_len = uart_read_bytes(UART_NUM_0, rx_head, HEAD_LEN, OS_100ms * 5);
    int read_len = uart_read_bytes(UART_NUM_0, rx_head, HEAD_LEN, portMAX_DELAY);

    if (checkPacketHead(rx_head) == false) {
        // printUart1("[%8d] RX0 Header BAD = %d/%d (ME = %d) \n", millis(), read_len, event_size, MY_BOARD_ID);
        // delay(5);
        printUart1("[%8d]\t {%d}Header BAD  %d, %d, %d, %d : %d, %d, %d, %d \n", millis(), 10,
                   rx_head[0], rx_head[1], rx_head[2], rx_head[3],
                   rx_head[4], rx_head[5], rx_head[6], rx_head[7]);

        // printPacketHeader(rx_head, 10);
        uart_flush_input(UART_NUM_0);  // RX 버퍼 memset

        return -1;
    } else {
        printUart1("[%8d] RX0 Head GOOD to %d, (%d/%d)\n", millis(), rx_head[IDX_MSG_ID], read_len, event_size);
    }

    int rx_TX_BOARD_ID = rx_head[IDX_TX_BOARD_ID];
    int rx_GROUP_ID = rx_head[IDX_GROUP_ID];
    int rx_MSG_ID = rx_head[IDX_MSG_ID];

    switch (rx_GROUP_ID) {
        case G_DEVICE_IO:      // no RX on uart0
        case G_SENSOR_DATA: {  // no RX on uart0
            return -10;
        } break;

        case G_OSD_COMMAND: {  // in case uart0, it's only for the Main
            int msg_id = rx_head[IDX_MSG_ID];
            int rx_board_id = (msg_id & 0x0F);

            if (rx_board_id == M_BOARD_0) {
                currentRxBuffer = packetBuf_OSD;  // in order to deliver to the sub
            } else {
                currentRxBuffer = packetBuf_OSDSub;  // in order to deliver to the sub
            }

            //  PARSE OSD DATA
            int ret_val = parsePacket_OSD_byMain(MY_BOARD_ID, rx_head, currentRxBuffer, isSubOSD_Filled);

            indi_1_r = 0;
            indi_1_g = 0;
            indi_1_b = 0;
            switch (ret_val) {
                case 0:  // OK
                    printUart1("OSD OK\n");
                    indi_1_g = 20;
                    break;
                case -1:  // length error
                    indi_1_r = 20;
                    break;
                case -10:  // tail error, r+g = yellow
                    indi_1_r = 20;
                    indi_1_g = 20;
                    break;
                default:  // warning
                    indi_1_b = 20;
                    break;
            }

        } break;

        case G_APP_COMMAND:  // NOT USE IT NOW.  it's only for the Main
            break;
        default:
            printUart1("[%8d]>> ERROR, WRONG GROUP ID = %d \n", rx_GROUP_ID);
            return -2;
    }

    // uart0_println("Packet received successfully");

    return 0;
}

int processRX1_SubDebug(int event_size) {
    uart_flush_input(UART_NUM_1);  // RX 버퍼 memset

    byte *currentRxBuffer;

    currentRxBuffer = packetBuf_OSD;
    memset(currentRxBuffer, 0, PACKET_LEN_OSD);

    if (true) {
        int read_len = uart_read_bytes(UART_NUM_1, currentRxBuffer, event_size, portMAX_DELAY);
        uart0_printf("(%d/%d)%s\n", read_len, event_size, currentRxBuffer);

        return 0;
    }

    int read_len = uart_read_bytes(UART_NUM_1, currentRxBuffer, PACKET_LEN_OSD, portMAX_DELAY);
    uart0_printf("(%d/%d), head {%d, %d}\n", read_len, event_size,
                 currentRxBuffer[0], currentRxBuffer[1]);

    // read_len = uart_read_bytes(UART_NUM_1, currentRxBuffer + 8, PACKET_LEN_OSD - 8, portMAX_DELAY);
    uart0_printf("(%d/%d), tail {%d, %d}\n", read_len, PACKET_LEN_OSD - 8,
                 currentRxBuffer[PACKET_LEN_OSD - 2], currentRxBuffer[PACKET_LEN_OSD - 1]);

    if ((checkPacketHead(currentRxBuffer) == true) && (currentRxBuffer[PACKET_LEN_OSD - 1] == TAIL_SYNC))
        copyPacketToOSDBuf(MY_BOARD_ID, currentRxBuffer, (currentRxBuffer + HEAD_LEN));
    else {
        uart0_printf("ERROR {%d, %d ~ %d}\n",
                     currentRxBuffer[0], currentRxBuffer[1], currentRxBuffer[PACKET_LEN_OSD - 1]);

        byte *cp = currentRxBuffer;

        uart0_printf("[HEAD] %d, %d, %d, %d | %d, %d, %d, %d | %d, %d, %d, %d | %d, %d, %d, %d \n",
                     cp[0], cp[1], cp[2], cp[3], cp[4], cp[5], cp[6], cp[7],
                     cp[8], cp[9], cp[10], cp[11], cp[12], cp[13], cp[14], cp[15]);

        int j = 16;
        while (true) {
            uart0_printf("[%02d] %3d, %3d, %3d, %3d | %3d, %3d, %3d, %3d | %3d, %3d, %3d, %3d | %3d, %3d \n", j,
                         cp[j + 0], cp[j + 1], cp[j + 2], cp[j + 3], cp[j + 4], cp[j + 5], cp[j + 6], cp[j + 7],
                         cp[j + 8], cp[j + 9], cp[j + 10], cp[j + 11], cp[j + 12], cp[j + 13]);
            j += 14;
            if ((HEAD_LEN + SUB_HEAD_LEN + PACKET_LEN_OSD_BODY) <= j)
                break;
        }

        uart0_printf("[TAIL] %d, %d\n",
                     cp[HEAD_LEN + SUB_HEAD_LEN + PACKET_LEN_OSD_BODY + 0],
                     cp[HEAD_LEN + SUB_HEAD_LEN + PACKET_LEN_OSD_BODY + 1]);

        uart_flush_input(UART_NUM_1);
    }

    // memset(currentRxBuffer, 0, PACKET_LEN_OSD);

    return 0;
}

int processRX1(byte *rx1_packet, int rx1_packet_size) {
    uart_flush_input(UART_NUM_1);  // RX 버퍼 memset

    byte *rx_head = rx1_packet;
    byte *bufferToParse = rx1_packet + HEAD_LEN;
    // memset(rx_head, 0, HEAD_LEN);

    // int read_len = uart_read_bytes(UART_NUM_1, rx_head, HEAD_LEN, OS_100ms * 5);
    // int read_len = uart_read_bytes(UART_NUM_1, rx_head, HEAD_LEN, portMAX_DELAY);
    int read_len = rx1_packet_size;

    if (checkPacketHead(rx_head) == false) {
        uart0_printf("[%8d] RX1 Header BAD- size = %d/%d (ME = %d) \n", millis(), read_len, rx1_packet_size, MY_BOARD_ID);
        printPacketHeader(rx_head, 10);

        // memset(packetBuf_Debug, 0, 256);
        // if (HEAD_LEN <= rx1_packet_size) {
        //     uart_read_bytes(UART_NUM_1, packetBuf_Debug + HEAD_LEN, rx1_packet_size - HEAD_LEN, portMAX_DELAY);
        //     memcpy(packetBuf_Debug, rx_head, HEAD_LEN);
        //     printBuf(packetBuf_Debug, rx1_packet_size, 38);
        // }
        // uart_flush_input(UART_NUM_1);  // RX 버퍼 memset

        return -1;
    }
    // uart0_printf("[%8d] RX1 Header GOOD, size = %d, [2]%d \n", millis(), read_len, rx_head[2]);

    int rx_TX_BOARD_ID = rx_head[IDX_TX_BOARD_ID];
    int rx_GROUP_ID = rx_head[IDX_GROUP_ID];
    int rx_MSG_ID = rx_head[IDX_MSG_ID];

    switch (rx_GROUP_ID) {
        case G_DEVICE_IO: {  // main : only tx,  sub : only rx
            switch (rx_MSG_ID) {
                case M_PERMIT:
                    // read_len = uart_read_bytes(UART_NUM_1, rx_head + HEAD_LEN, TAIL_LEN, portMAX_DELAY);

                    parsePacket_Permit(rx_head, nullptr, MY_BOARD_ID, rs485Bus_granted);
                    uart0_printf("<%d>[%8d] 485 RX, PERMIT (%d to = %d), Grant = %d,  [2]%d \n", MY_BOARD_ID, millis(),
                                 rx_TX_BOARD_ID, rx_head[IDX_PERMIT_ID], rs485Bus_granted, rx_head[IDX_VER]);
                    // printPacketHeader(rx_head, 11);
                    break;

                default:
                    break;
            }
            return 0;
        } break;

        case G_SENSOR_DATA: {  // main : only rx,  sub : tx, rx
            // bufferToParse = packetBuf_SensorSub;
            // uart0_printf("\n[%8d] RX1, Sensor {Sender:%d ==> ME} [2]%d \n",
            //              millis(), rx_head[IDX_TX_BOARD_ID], rx_head[IDX_VER]);

            //  PARSE SENSOR DATA
            parsePacket_Sensor_1Bd(MY_BOARD_ID, rx_head, bufferToParse);

            workAfter_Sensor(rx_TX_BOARD_ID);  // do nothing
            return 0;
        } break;

        case G_OSD_COMMAND: {  // in case 485, G_OSD_COMMAND is only for the sub
            // bufferToParse = packetBuf_OSD;

            //  PARSE OSD DATA
            parsePacket_OSD_bySub(MY_BOARD_ID, rx_head, bufferToParse);

        } break;

        case G_APP_COMMAND:  // NOT USE IT NOW. main : only tx,  sub : only rx
            break;

        default:
            uart0_printf("[%8d]>> ERROR, WRONG GROUP ID = %d \n", rx_GROUP_ID);
            return -2;
    }

    // RX 데이터 정상 수신 후 추가 이벤트 제거
    clear_uart_event_queue(UART1_EventQueue);

    // uart0_println("Packet received successfully");

    return 0;
}

//  main task on core 0
void pumpSerial(void *pParam) {
    uart0_printf("PUMP- ENTER\n");
    // esp_task_wdt_delete(NULL);  // 현재 태스크를 WDT 감시에서 제외

    int ret_val = 0;

    while (true) {
        vTaskDelay(1);

        //  FOR TESTING !!!
        // adc_scan_done = true;
        rs485Bus_granted = true;

        // uart0_printf("[%8d] PUMP loop = %d (dip=%d, adc t/f=%d) \n", millis(), pump_count, MY_BOARD_ID, adc_scan_done);

        if (MY_BOARD_ID == 0) {
            // if (isSubOSD_Filled == true) {
            //     sendPacket1(packetBuf_OSDSub, PACKET_LEN_OSD);
            //     isSubOSD_Filled = false;
            // }

            if (adc_scan_done == false) {
                continue;
            }

            buildPacket_Sensor_1Set(MY_BOARD_ID, packetBuf, forceBuffer_rd, NUM_1SET_SEN_WIDTH, NUM_1SET_SEN_HEIGHT);
            sendPacket0(packetBuf, PACKET_LEN_SEN_1SET);  // it consumes 11ms per one board. so for 2, it consumes 22ms.

            //  permit 패킷은 논리적으로는 여기에서 보내야 함.
            {
                // reser_1 = (reser_1 + 1) % 100;
                // buildPacket_Permit(packetBuf_DeviceIO, M_BOARD_1);
                // sendPacket1(packetBuf_DeviceIO, HEAD_LEN + TAIL_LEN);
                // tick_permit_last = millis();
            }

            adc_scan_done = false;  // sendPacket0 앞으로 보내면 led 노이즈 발생함. 도저히 이해 불가

        } else {
            if ((adc_scan_done == false) || (rs485Bus_granted == false)) {
                continue;
            }

            buildPacket_Sensor_1Bd(MY_BOARD_ID, packetBuf, forceBuffer_rd, SIZE_X, SIZE_Y);
            sendPacket1(packetBuf, PACKET_LEN_SEN_1Bd);
            vTaskDelay(1);
            /*
                        sendPacket1(packetBuf, PACKET_LEN_SEN_1Bd);
                        vTaskDelay(1);
                        sendPacket1(packetBuf, PACKET_LEN_SEN_1Bd);
                        vTaskDelay(1);
                        sendPacket1(packetBuf, PACKET_LEN_SEN_1Bd);
                        vTaskDelay(1);
                        sendPacket1(packetBuf, PACKET_LEN_SEN_1Bd);
                        vTaskDelay(1);
                        sendPacket1(packetBuf, PACKET_LEN_SEN_1Bd);
                        vTaskDelay(1);
                        sendPacket1(packetBuf, PACKET_LEN_SEN_1Bd);
            */
            // vTaskDelay(11);  // led dma 전송 중에 noise 발생, 메인 보드와 sync 맞추기 위해 11ms 대기

            rs485Bus_granted = false;
            adc_scan_done = false;
        }

        pump_count++;
    }  // while

    return;
}

#endif  // _COMMPACKET_H_