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

#include <esp_log.h>
// #include "esp_task_wdt.h"
#include "esp_timer.h"
#include "hal/uart_ll.h"
#include "soc/uart_reg.h"
#include "soc/uart_struct.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
// #include "lib_ble_ota.h"
#include "configPins-mdll-24-6822.h"
#include "lib_gpio.h"

#include "libProtocol.h"
#include "groupDeviceIO.h"
#include "groupOSDCommand.h"
#include "groupSensorData.h"
#include "packetBuffer.h"

// #include "lib_rle_util.h"
// #include "lib_rle.h"

static const char *TAG = "UART_TASK";

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
int BoardID_MAX = 7;
extern int adc_scan_count_main;
extern int adc_scan_done;

int pump_count = 0;
int tick_permit_last = 0;

int processRX0(int event_size);
int processRX1(byte *rx1_packet, int rx1_packet_size);
int processRX1_SubDebug(int rx1_packet_size);

bool isBoard0_UART1_using = false;  // UART1 사용중에 ADC 측정하면 송수신 노이즈 발생. (ADC 1회 측정이 30us 인데, 이때 UART1 인터럽트 에러남)
int indexPermit = 0;

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


int64_t wait_tx_done_async(int uart_num, int max_wait_duration_ms) {
    int64_t lap_enter = esp_timer_get_time();  // 마이크로초 단위 시간 측정

    // while (true) {
        // esp_err_t result = uart_wait_tx_done(uart_num, 0);  // 즉시 반환 모드 0 or portMAX_DELAY
        // esp_err_t result = uart_wait_tx_done(uart_num, portMAX_DELAY);  // 즉시 반환 모드 0 or portMAX_DELAY
        esp_err_t result = uart_wait_tx_done(uart_num, pdMS_TO_TICKS(max_wait_duration_ms)); // 타임아웃 설정 가능

        int elapsed_time = esp_timer_get_time() - lap_enter;

        if (result == ESP_OK)
            return elapsed_time;  // ==> 주로 여기에서 반환됨. 3500 ~ 4800us . 반면에 수신은 2800us 정도.

        // in case (result != ESP_OK)
        // if (max_wait_duration_ms * 1000 < elapsed_time) {
            uart0_printf("[%8d] TX Timeout (%8lld us)\n", millis(), elapsed_time);
            return elapsed_time;
        // }

    // }

    return max_wait_duration_ms * 1000 + 1;
}

//  send data to the PC
int tx0_count = 0;
void sendPacket0(byte *packet_buffer, int packet_len) {
    int cur_time = millis();
    uart0_printf("[%8d] TX0, go %dth \n", millis(), pump_count);

    uart_write_bytes(UART_NUM_0, packet_buffer, packet_len);  // 이거 non-blocking으로 바꿔야 함.
    uart0_printf("\n");

    int time_dur_ms = 50;  // 11ms per one board
    int tx_dur_us = wait_tx_done_async(0, time_dur_ms);
    if (time_dur_ms <= (tx_dur_us / 1000))
        uart0_printf("[%8d] TX0, Time Err %dus \n", millis(), tx_dur_us);

    uart0_printf("[%8d] TX0, size= %d, dur=%d ms, %dth \n", millis(), packet_len, millis() - cur_time, pump_count);
    // printPacket(packet_buffer, packet_len);

    tx0_count++;
}

void setDE(bool state) {
    if (state) {
        disable_uart_rx_events(UART_NUM_1);  // RX 이벤트 비활성화

        // vTaskDelay(4);  // TX 모드 전환 이전 안정화 지연

        delayMicroseconds(10 + 10);
        digitalWrite(pin485U1DE, HIGH);  // LOW : RX, HIGH : TX
        // delayMicroseconds(650 + 50 + 800);     // 안정화 지연
        delayMicroseconds(650 + 00);     // 안정화 지연. tx and 650. 150 and rx.
    } else {
        // delayMicroseconds(250 + -50 + 800);
        delayMicroseconds(150); // 안정화 지연. tx and 650. 150 and rx.
        digitalWrite(pin485U1DE, LOW);  // LOW : RX, HIGH : TX

        // delayMicroseconds(10 + -7);

        uart_flush_input(UART_NUM_1);
        delayMicroseconds(10);  // 플러시 후 안정화 딜레이 추가

        enable_uart_rx_events(UART_NUM_1);  // RX 이벤트 다시 활성화
    }
}

SemaphoreHandle_t semaTX1;  // 세마포어 핸들 생성

//  send data via 485 BUS (Half duplex)
bool sendPacket1(byte *packet_buffer, int packet_len) {
    // uart0_printf("[%11lldus] TX1 go \n", esp_timer_get_time());
    if (xSemaphoreTake(semaTX1, portMAX_DELAY) == pdTRUE) {
        int tx_len = 0;

        setDE(true);  // 송신 모드 전환
    
        int64_t cur_snap = esp_timer_get_time();  // 마이크로초 단위로 타이머 초기화
    
        tx_len = uart_write_bytes(UART_NUM_1, packet_buffer, packet_len);
        int64_t core_tx_dur_us = wait_tx_done_async(UART_NUM_1, 40);  // uart_num, max_wait_duration_ms
    
        int64_t duration_us = esp_timer_get_time() - cur_snap;
    
        setDE(false);  // 수신 모드 전환

        // uart0_printf("[%11lldus] TX1 (USE %5lld us in DUR %5lld us), len=%4d, { [2]%3d, [7]%3d } \n",
        //              esp_timer_get_time(), core_tx_dur_us, duration_us,
        //              tx_len, packet_buffer[2], packet_buffer[7]);

        xSemaphoreGive(semaTX1);  // 세마포어 해제
    }


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

void uart1_event_task(void *pvParameters) {
    int64_t pure_read_init_us = 0;
    int64_t read_dur_us_pure = 0;
    int64_t one_packet_read_total_us_pure = 0;

    int one_packet_len = 0;
    int one_packet_event_count = 0;
    int64_t one_packet_read_init_us = 0;   // us 단위로 변경
    int64_t one_packet_read_total_us = 0;  // us 단위로 변경
    int packet_sensor_count = 0;

    int packet_group_id = 0;
    int packet_msg_id = 0;
    int packet_to_read = 0;

    uart0_printf("UART 1 RX ENTER - EVENT:%d, %d, %d, %d \n", UART_DATA, UART_BREAK, UART_BUFFER_FULL, UART_FIFO_OVF);

    byte *currentRxBuffer = packetBuf_RX1;  // temporary
    byte *eventBuffer = (byte *)malloc(128);

    enable_uart_rx_events(UART_NUM_1);  // RX 이벤트 활성화

    uart_event_t event;
    while (true) {
        if (xQueueReceive(UART1_EventQueue, (void *)&event, portMAX_DELAY)) {
            // uart0_printf("[%8d] RX1 Event=%d, len=%d \n", millis(), event.type, event.size);
            // uart0_printf("[%11lldus] RX1 Event=%d, len=%d \n", esp_timer_get_time(), event.type, event.size);

            switch (event.type) {
                case UART_DATA: {
                    pure_read_init_us = esp_timer_get_time();
                    int read_len = uart_read_bytes(UART_NUM_1, eventBuffer, event.size, 5 / portTICK_PERIOD_MS);
                    read_dur_us_pure = esp_timer_get_time() - pure_read_init_us;

                    if (eventBuffer[0] == HEADER_SYNC && eventBuffer[1] == HEADER_SYNC) {
                        // uart0_printf("[%8d] rx1 go [2]%d \n", millis(), eventBuffer[2]);
                        packet_group_id = eventBuffer[IDX_GROUP_ID];
                        packet_msg_id = eventBuffer[IDX_MSG_ID];

                        switch (packet_group_id) {
                            case G_DEVICE_IO:
                                switch (packet_msg_id) {
                                    case M_PERMIT:
                                        packet_to_read = 10;
                                        break;
                                    default:
                                        /* code */
                                        break;
                                }
                                break;
                            case G_SENSOR_DATA:
                                packet_to_read = 962;
                                break;
                            case G_OSD_COMMAND:
                                packet_to_read = eventBuffer[IDX_LENGTH_100] * 100 + eventBuffer[IDX_LENGTH_0];
                                break;

                            default:
                                /* code */
                                break;
                        }

                        one_packet_event_count = 0;
                        one_packet_len = 0;
                        one_packet_read_init_us = pure_read_init_us;  // 마이크로초 단위로 타이머 초기화
                        one_packet_read_total_us_pure = 0;

                        memset(currentRxBuffer, 0, PACKET_LEN_SEN_1Bd);
                    }

                    memcpy(currentRxBuffer + one_packet_len, eventBuffer, event.size);
                    one_packet_len += event.size;
                    one_packet_event_count++;

                    if (packet_to_read < one_packet_len) {
                        uart0_printf("[%11lldus] size:%d, RX1 Times: %d, %d in Q, size: %d [2]%d ++++ ++++ ++++ Long Error ++++ ++++ ++++ ++++\n",
                                    esp_timer_get_time(), event.size, one_packet_event_count, 0, one_packet_len, currentRxBuffer[2]);
                        uart0_printf("\t Cur buffer) [0]%4d [1]%4d [2]%4d [3]%4d ~ [%d]%4d \n",
                                    eventBuffer[0], eventBuffer[1], eventBuffer[2], eventBuffer[3], event.size - 1, eventBuffer[event.size - 1]);
                    }

                    one_packet_read_total_us_pure += read_dur_us_pure;
                    one_packet_read_total_us = esp_timer_get_time() - one_packet_read_init_us;

                    int queue_num = 0;
                    if (eventBuffer[event.size - 1] == TAIL_SYNC) {
                        queue_num = uxQueueMessagesWaiting(UART1_EventQueue);
                        // if ((currentRxBuffer[2] % 100 == 0) || (currentRxBuffer[2] % 100 == 1))
                        // uart0_printf("[%8d] RX1 (USE %5lld us in DUR %5lld us), len=%d, 1~%d,%2dea in Q [2]%3d\n",
                        //              millis(), one_packet_read_total_us_pure, one_packet_read_total_us,
                        //              one_packet_len, one_packet_event_count, queue_num, currentRxBuffer[2]);

                        // 온갖 테스트 코드
                        if(true)
                        {
                            // setDE(true);  // 송신 모드 전환
                            // setDE(false);  // 수신 모드 전환

                            packet_sensor_count++;
                            if(packet_sensor_count % 100 == 0){
                                uart0_printf(".");
                                
                                if(packet_sensor_count %10000 == 0){
                                    uart0_printf("\n");
                                }
                            }
                        }

                        //  reset log variables
                        one_packet_event_count = 0;
                        one_packet_len = 0;
                        one_packet_read_init_us = pure_read_init_us;  // 마이크로초 단위로 타이머 초기화
                        one_packet_read_total_us = 0;
                        one_packet_read_total_us_pure = 0;
                        memset(eventBuffer, 0, 128);

                        processRX1(currentRxBuffer, one_packet_len);  // 패킷 처리
                    } else {
                        if (9 < one_packet_event_count) {  // 9  is the maximum number of events in one packet.
                            uart0_printf("\n[%11lldus] RX1 Total=%5lld us, Times: %d, %d in Q, len: %d [2]%d ++++ WRONG ++++\n",
                                esp_timer_get_time(), one_packet_read_total_us, one_packet_event_count, queue_num, one_packet_len, currentRxBuffer[2]);
                            // printBuf(currentRxBuffer + one_packet_len - event.size, event.size, one_packet_event_count);
                            xQueueReset(UART1_EventQueue);
                            uart_flush(UART_NUM_1);
                        }
                    }
                    // uart0_printf("\n[%8d]UART1 UART_DATA done \n", millis());

                } break;
                case UART_BREAK:
                case UART_BUFFER_FULL:
                case UART_FIFO_OVF: {
                    int queue_num = uxQueueMessagesWaiting(UART1_EventQueue);

                    uart0_printf("\n[%11lldus] RX1 ERROR ++++ ++++ ++++ type=%d, len=%d (1:BREAK, 2:FULL, 3:Overflow) ++++ ++++ ++++ ++++ \n", 
                                esp_timer_get_time(), event.type, event.size);

                    if (event.type == UART_BREAK) {
                        // if (4 < event.size) 
                        {
                            int read_len = uart_read_bytes(UART_NUM_1, eventBuffer, event.size, 5 / portTICK_PERIOD_MS);
                            // uart0_printf("\t UART_BREAK) [0]%4d [1]%4d [2]%4d [3]%4d ~ [%d]%4d \n",
                            //              eventBuffer[0], eventBuffer[1], eventBuffer[2], eventBuffer[3], event.size - 1, eventBuffer[event.size - 1]);
                            uart0_printf("\t UART_BREAK) ");
                            for (int i = 0; i < event.size; i++) {
                                uart0_printf("[%d]%4d ", i, eventBuffer[i]);
                            }
                            uart0_printf("\n");
                        }
                    }
                    else { // // UART_BUFFER_FULL, UART_FIFO_OVF
                        xQueueReset(UART1_EventQueue);
                        uart_flush(UART_NUM_1);
                        // // RX 이벤트 큐 정리 (불필요한 이벤트 제거)
                        //         clear_uart_event_queue(UART1_EventQueue);
                    }

                    // uart0_printf("[%8d] RX1 (USE %5lld us in DUR %5lld us), len=%d, 1~%d,%2dea in Q [2]%3d\n",
                    //     millis(), one_packet_read_total_us_pure, one_packet_read_total_us,
                    //     event.size, one_packet_event_count, queue_num, eventBuffer[2]);

                }   break;
                case UART_PATTERN_DET:
                    uart0_println("[%11lldus]UART1 Event : %d, len=%d", esp_timer_get_time(), event.type, event.size);
                    uart_flush_input(UART_NUM_1);
                    break;
                default:
                    uart0_println("[%11lldus]UART1 Event : %d, len=%d", esp_timer_get_time(), event.type, event.size);
                break;
            }  // switch (event.type)
        }  // if (xQueueReceive(UART1_EventQueue, (void *)&event, portMAX_DELAY))

        // uart0_printf("[%11lld] RX1 event end...\n", esp_timer_get_time());
        vTaskDelay(0);  // 아무 이벤트가 없으면 CPU 점유권을 넘김

    }  // while (true)
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

    int read_len = rx1_packet_size;

    if (checkPacketHead(rx_head) == false) {
        uart0_printf("[%8d] RX1 Header BAD- size = %d/%d (ME = %d) \n", millis(), read_len, rx1_packet_size, MY_BOARD_ID);
        printPacketHeader(rx_head, 10);

        return -1;
    }

    int rx_TX_BOARD_ID = rx_head[IDX_TX_BOARD_ID];
    int rx_GROUP_ID = rx_head[IDX_GROUP_ID];
    int rx_MSG_ID = rx_head[IDX_MSG_ID];

    switch (rx_GROUP_ID) {
        case G_DEVICE_IO: {  // main : only tx,  sub : only rx
            switch (rx_MSG_ID) {
                case M_PERMIT:
                    parsePacket_Permit(rx_head, nullptr, MY_BOARD_ID, rs485Bus_granted);
                    uart0_printf("[%8d] RX1 PERMIT (%d to %d)  [2]%3d \n", millis(),
                                 rx_TX_BOARD_ID, rx_head[IDX_PERMIT_ID], rx_head[IDX_VER]);

                    indexPermit = rx_head[IDX_DATA_1];
                    break;

                default:
                    break;
            }
            return 0;
        } break;

        case G_SENSOR_DATA: {  // main : only rx,  sub : tx, rx
            // bufferToParse = packetBuf_SensorSub;

            //  PARSE SENSOR DATA
            int tx_board_id;
            parsePacket_Sensor_1Bd(MY_BOARD_ID, rx_head, bufferToParse, tx_board_id);

            // uart0_printf("\n[%8d] RX1, Sensor {Sender:%d ==> ME} [2]%d \n",
            //              millis(), tx_board_id, rx_head[IDX_VER]);

            if (BoardID_MAX == tx_board_id) {
                isBoard0_UART1_using = false;
                uart0_printf("u");

                // uart0_printf("[%8d] RX1 Sensor completed = %d, [2]=%d \n\n", millis(), tx_board_id, rx_head[2]);
            }

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
    // clear_uart_event_queue(UART1_EventQueue);

    return 0;
}

SemaphoreHandle_t semaSendPermit;  // 세마포어 핸들 생성

void considerToPermit(int permit_interval) {
    if (MY_BOARD_ID != 0)
        return;

    // permit_interval is 1 or 120
    if ((millis() - tick_permit_last) < permit_interval) {
        // if (isSubOSD_Filled == true)
        //     continue;
        return;
    }

    if (xSemaphoreTake(semaSendPermit, portMAX_DELAY) == pdTRUE) {
        reser_1 = (reser_1 + 1) % 200;
        buildPacket_Permit(packetBuf_DeviceIO, M_BOARD_1);  // M_BOARD_1 : destination board id

        isBoard0_UART1_using = true;
        sendPacket1(packetBuf_DeviceIO, HEAD_LEN + TAIL_LEN);

        // uart0_printf("[%8d] check permit{%d} %d ms (last=%d) [7]%d \n", millis(), permit_interval, millis() - tick_permit_last,
        //             tick_permit_last, reser_1);
        tick_permit_last = millis();

        xSemaphoreGive(semaSendPermit);
    }
}

//  main task on core 0
void pumpSerial(void *pParam) {
    uart0_printf("[%8d]PUMP- ENTER\n", millis());
    // esp_task_wdt_delete(NULL);  // 현재 태스크를 WDT 감시에서 제외

    int ret_val = 0;

    while (true) {
        vTaskDelay(1); // pumpSerial() task 초입.

        // uart0_printf("[%8d] PUMP loop = %d (dip=%d, adc t/f=%d) \n", millis(), pump_count, MY_BOARD_ID, adc_scan_done);

        if (MY_BOARD_ID == 30) { // FAKE for TESTING
            if (adc_scan_done != true) {
                continue;
            }
            if (isBoard0_UART1_using == true) {
                continue;
            }

            adc_scan_done = false;  // turn on loopADCRead, it will give sema to draw led task.
        }
        else if (MY_BOARD_ID == 0) {
            // if (isSubOSD_Filled == true) {
            //     sendPacket1(packetBuf_OSDSub, PACKET_LEN_OSD);
            //     isSubOSD_Filled = false;
            // }

            if (adc_scan_done != true) {
                continue;
            }
            if (isBoard0_UART1_using == true) {
                continue;
            }
            uart0_printf("U");


            if((pump_count % 2) == 1) {
                buildPacket_Sensor_1Bd(MY_BOARD_ID, packetBuf, forceBuffer_rd, SIZE_X, SIZE_Y);
                sendPacket1(packetBuf, PACKET_LEN_SEN_1Bd);               
            } else {
            }

            //  permit to board
            considerToPermit(1);  //  1 : 1ms interval

            // Send Sensor Data to UART0
            buildPacket_Sensor_1Set(MY_BOARD_ID, packetBuf, forceBuffer_rd, NUM_1SET_SEN_WIDTH, NUM_1SET_SEN_HEIGHT);
            // sendPacket0(packetBuf, PACKET_LEN_SEN_1SET);  // it consumes 11ms per one board. so for 2, it consumes 22ms.

            adc_scan_done = false;  // turn on loopADCRead, it will give sema to draw led task.

        } else {
            // rs485Bus_granted = true; // FAKE for TESTING
            if ((adc_scan_done != true) || (rs485Bus_granted != true)) {
                continue;
            }
            
            isBoard0_UART1_using = true;
            uart0_printf("\nU\n");

            buildPacket_Sensor_1Bd(MY_BOARD_ID, packetBuf, forceBuffer_rd, SIZE_X, SIZE_Y);
            sendPacket1(packetBuf, PACKET_LEN_SEN_1Bd);

            packetBuf[3] = 1;
            sendPacket1(packetBuf, PACKET_LEN_SEN_1Bd);
            packetBuf[3] = 7;
            sendPacket1(packetBuf, PACKET_LEN_SEN_1Bd);

            vTaskDelay(0);  // context switch
            uart0_printf("u");
            isBoard0_UART1_using = false;

            rs485Bus_granted = false;
            adc_scan_done = false;
        }

        pump_count++;
    }  // while

    return;
}

#endif  // _COMMPACKET_H_