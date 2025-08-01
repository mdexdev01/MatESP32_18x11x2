/*
  <Arduino Config>
  BOARD : "ESP32-S3 DEVKITC-1" (NOT 1.1) OR "ADAFRUIT QT py esp32-C3"
  DESC : SRAM 512K, ROM 384K, FLASH 16M, PSRAM 8M
  CONFIG : Flash QIO 80MHz, OPI PSRAM, Partition Minimal SPIFFS, others : apply default setting

  SEE 74HC595 later - https://docs.arduino.cc/tutorials/communication/guide-to-shift-out
*/

#ifndef _COMMPACKET_H_
#define _COMMPACKET_H_

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
#include "groupAppCommand.h"
#include "groupOSDCommand.h"
#include "groupSensorData.h"
#include "packetBuffer.h"

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

TaskHandle_t receiverTaskHandle;

//---------------------------------------------
//  Declarations
//---------------------------------------------
int BoardID_MAX = 7;

bool isSensorDataFilled = false;

int pumpCount = 0;
int tickPermitLast = 0;

int processRX0(int event_size);
int processRX1(byte *rx1_packet, int rx1_packet_size);

SemaphoreHandle_t semaTX1;  // 세마포어 핸들 생성

bool isBoard0_duringPermitCycle = false;  // UART1 사용중에 ADC 측정하면 송수신 노이즈 발생. (ADC 1회 측정이 30us 인데, 이때 UART1 인터럽트 에러남)
int indexPermit = 0;


bool isPermitGranted = false;

extern int indi_1_r;
extern int indi_1_g;
extern int indi_1_b;

//---------------------------------------------
//  UART0 EVENT HANDLER
//---------------------------------------------

void disable_uart_rx_events(int uart_num);
void enable_uart_rx_events(int uart_num);

//---------------------------------------------
//  Function Definitions
//---------------------------------------------
void disable_uart_rx_events(int uart_num) {
    uart_disable_intr_mask(uart_num, UART_RXFIFO_FULL_INT_ENA_M | UART_RXFIFO_TOUT_INT_ENA_M | UART_BRK_DET_INT_ENA_M);
}

void enable_uart_rx_events(int uart_num) {
    uart_enable_intr_mask(uart_num, UART_RXFIFO_FULL_INT_ENA_M | UART_RXFIFO_TOUT_INT_ENA_M | UART_BRK_DET_INT_ENA_M);
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
    // uart0_printf("[%8d] TX0, go %dth \n", millis(), pumpCount);

    uart_write_bytes(UART_NUM_0, packet_buffer, packet_len);  // 이거 non-blocking으로 바꿔야 함.

    int time_dur_ms = 50;  // 11ms per one board
    int tx_dur_us = wait_tx_done_async(0, time_dur_ms);
    if (time_dur_ms <= (tx_dur_us / 1000))
        uart0_printf("[%8d] TX0, Time Err %dus \n", millis(), tx_dur_us);

    // uart0_printf("[%8d] TX0, size= %d, dur=%d ms, %dth \n", millis(), packet_len, millis() - cur_time, pumpCount);
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
            printUart1("#\n[%d]UART0 Event : %d, size=%d \n", MY_BOARD_ID, event.type, event.size);
            
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

void receiverTask(void* param) { // tcpReceiverTask로 대체 예정
    byte *currentRxBuffer;

    while (true) {
/*        

        if(isTCP_Connected == false) {
            vTaskDelay(200 / portTICK_PERIOD_MS);  // 200ms 대기 후 재시도
            continue;    
        }
        // if(pTcpClient == NULL) {
        //   vTaskDelay(200 / portTICK_PERIOD_MS);  // 1초 대기 후 재시도
        //   continue;    
        // }

        // if (false == pTcpClient->connected()){
        //   vTaskDelay(200 / portTICK_PERIOD_MS);  // 200ms 대기 후 재시도
        //   continue;
        // }

        if (PACKET_SIZE <= pTcpClient->available()) {
            uint8_t rxBuf[PACKET_SIZE];
            int read_len = pTcpClient->read(rxBuf, HEAD_LEN);

            uart0_printf("[RX] Packet received from PC, size: %d \n", read_len);

            if (rxBuf[IDX_HEADER_0] == HEADER_SYNC && rxBuf[IDX_HEADER_1] == HEADER_SYNC) {
                uart0_printf("Header Pass \n");

                // 필요한 처리를 여기에 추가 가능
                int msg_id = rxBuf[IDX_MSG_ID];
                int rx_board_id = (msg_id & 0x0F);
                int size_100 = rxBuf[IDX_LENGTH_100];  // OSD LENGTH / 100 (SubHeader + Body, No Tail)
                int size_1 = rxBuf[IDX_LENGTH_1];      // OSD LENGTH % 100 (SubHeader + Body, No Tail)
                int body_len = size_100 * 100 + size_1;

                if (rx_board_id == M_BOARD_0) {
                    currentRxBuffer = packetBuf_OSD;  // in order to deliver to the sub
                } else {
                    currentRxBuffer = packetBuf_OSDSub;  // in order to deliver to the sub
                }

                memcpy(currentRxBuffer, rxBuf, HEAD_LEN);  // copy header

                read_len = pTcpClient->read(currentRxBuffer + HEAD_LEN, body_len);  // read body
                uart0_printf("Payload read size: %d \n", read_len);

                if(currentRxBuffer[HEAD_LEN + body_len - 1] == TAIL_SYNC) {
                    uint8_t seq = rxBuf[PACKET_SIZE - 2];
                    uart0_printf("[RX] OSD | seq: %d\n", currentRxBuffer[HEAD_LEN + body_len - 2]);
                }
                else {
                    uart0_printf("[RX] OSD | seq: %d\n", currentRxBuffer[HEAD_LEN + body_len - 2]);
                    continue;  // skip processing if packet error
                }

                // printUart1("#[%8d] Data len= %d, body [2]%d[3]%d ~ [-2]%d[-1]%d \n", millis(), 
                //         read_len, currentRxBuffer[2], currentRxBuffer[3], 
                //         currentRxBuffer[body_len-2], currentRxBuffer[body_len-1]);

                //  PARSE OSD DATA
                int ret_val = parsePacket_OSD_byMain(MY_BOARD_ID, rxBuf, currentRxBuffer + HEAD_LEN, isSubOSD_Filled);
                packetOSD_SizeSub = body_len + HEAD_LEN;

            }
            else { // ERROR - HEADER_SYNC 
                uart0_printf("[RX] Packet Error from PC, size: %d \n", read_len);
                uart0_printf("[RX] Packet Error from PC, data: [0]%d, [1]%d, [2]%d, [3]%d ~ [%d]%d \n",
                             rxBuf[0], rxBuf[1], rxBuf[2], rxBuf[3],
                             read_len - 1, rxBuf[read_len - 1]);
            }
        }
*/
        delay(1);
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
                                packet_to_read = eventBuffer[IDX_LENGTH_100] * 100 + eventBuffer[IDX_LENGTH_1] + HEAD_LEN;
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
                    } // if HEADER_SYNC
                    else if(eventBuffer[0] == '#') { // print log
                        uart0_printf("%s", eventBuffer);
                        break;
                    }

                    memcpy(currentRxBuffer + one_packet_len, eventBuffer, event.size);
                    one_packet_len += event.size;
                    one_packet_event_count++;

                    if (packet_to_read < one_packet_len) {
                        uart0_printf("[%11lldus] size:%d, RX1 Times: %d, %d in Q, size: %d/%d [2]%d ++++ ++++ ++++ Long Error ++++ ++++ ++++ ++++\n",
                                    esp_timer_get_time(), event.size, one_packet_event_count, 0, one_packet_len, packet_to_read, currentRxBuffer[2]);
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

                        processRX1(currentRxBuffer, one_packet_len);  // 패킷 처리

                        //  reset log variables
                        one_packet_event_count = 0;
                        one_packet_len = 0;
                        one_packet_read_init_us = pure_read_init_us;  // 마이크로초 단위로 타이머 초기화
                        one_packet_read_total_us = 0;
                        one_packet_read_total_us_pure = 0;
                        memset(eventBuffer, 0, 128);

                    } else {
                        if (17 < one_packet_event_count) {  // 9  is the maximum number of events in one packet.
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

                    // uart0_printf("\n[%11lldus] RX1 ERROR ++++ ++++ ++++ type=%d, len=%d (1:BREAK, 2:FULL, 3:Overflow) ++++ ++++ ++++ ++++ \n", 
                    //             esp_timer_get_time(), event.type, event.size);

                    if (event.type == UART_BREAK) {
                        // if (4 < event.size) 
                        {
                            int read_len = uart_read_bytes(UART_NUM_1, eventBuffer, event.size, 5 / portTICK_PERIOD_MS);
                            // uart0_printf("\t UART_BREAK) [0]%4d [1]%4d [2]%4d [3]%4d ~ [%d]%4d \n",
                            //              eventBuffer[0], eventBuffer[1], eventBuffer[2], eventBuffer[3], event.size - 1, eventBuffer[event.size - 1]);
                            uart0_printf("\t UART_BREAK) ");
                            for (int i = 0; i < event.size; i++) {
                                if( (HEAD_LEN < i) && (i <= event.size - 3) )
                                    continue;
                                uart0_printf("[%d]%4d ", i, eventBuffer[i]);
                            }
                            uart0_printf("\n");
                        }
                    }
                    else { // // UART_BUFFER_FULL, UART_FIFO_OVF
                        xQueueReset(UART1_EventQueue);
                        uart_flush(UART_NUM_1);
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
    if(MY_BOARD_ID != 0) {
        return;
    }
    memset(buffer, 0, 256);

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    sendPacket1((byte *)buffer, strlen(buffer));
    ets_delay_us(10);
    memset(buffer, 0, 256);
}


int processRX0(int event_size) { // handleSerialCommand로 대체 예정. tcpReceiverTask로 이동해야 함.
    // uart_flush_input(UART_NUM_0);  // RX 버퍼 memset

    byte *rx_head = packetHead;
    byte *currentRxBuffer;
    memset(rx_head, 0, HEAD_LEN);

    // int read_len = uart_read_bytes(UART_NUM_0, rx_head, HEAD_LEN, OS_100ms * 5);
    int read_len = uart_read_bytes(UART_NUM_0, rx_head, HEAD_LEN, portMAX_DELAY);

    if (checkPacketHead(rx_head) == false) {
        // printUart1("[%8d] RX0 Header BAD = %d/%d (ME = %d) \n", millis(), read_len, event_size, MY_BOARD_ID);
        // delay(5);
        printUart1("#[%8d]\t read_len=%d Header BAD  %d, %d, %d, %d : %d, %d, %d, %d \n", millis(), read_len, 
                   rx_head[0], rx_head[1], rx_head[2], rx_head[3],
                   rx_head[4], rx_head[5], rx_head[6], rx_head[7]);

        // printPacketHeader(rx_head, 10);
        uart_flush_input(UART_NUM_0);  // RX 버퍼 memset

        return -1;
    } else {
        printUart1("#[%8d] RX0 Head GOOD to %d, (%d/%d)\n", millis(), rx_head[IDX_MSG_ID], read_len, event_size);
    }

    int rx_TX_BOARD_ID = rx_head[IDX_TX_BOARD_ID];
    int rx_GROUP_ID = rx_head[IDX_GROUP_ID];
    int rx_MSG_ID = rx_head[IDX_MSG_ID];

    switch (rx_GROUP_ID) {
        case G_OSD_COMMAND: {  // in case uart0, it's only for the Main
            int msg_id = rx_head[IDX_MSG_ID];
            int rx_board_id = (msg_id & 0x0F);
            int size_100 = rx_head[IDX_LENGTH_100];  // OSD LENGTH / 100 (SubHeader + Body, No Tail)
            int size_1 = rx_head[IDX_LENGTH_1];      // OSD LENGTH % 100 (SubHeader + Body, No Tail)
            int body_len = size_100 * 100 + size_1;

            if (rx_board_id == M_BOARD_0) {
                currentRxBuffer = packetBuf_OSD;  // in order to deliver to the sub
            } else {
                currentRxBuffer = packetBuf_OSDSub;  // in order to deliver to the sub
            }

            memcpy(currentRxBuffer, rx_head, HEAD_LEN);  // copy header
            
            read_len = uart_read_bytes(UART_NUM_0, currentRxBuffer + HEAD_LEN, body_len, portMAX_DELAY);
            // printUart1("#[%8d] Data len= %d, body [2]%d[3]%d ~ [-2]%d[-1]%d \n", millis(), 
            //         read_len, currentRxBuffer[2], currentRxBuffer[3], 
            //         currentRxBuffer[body_len-2], currentRxBuffer[body_len-1]);

            //  PARSE OSD DATA
            int ret_val = parsePacket_OSD_byMain(MY_BOARD_ID, rx_head, currentRxBuffer + HEAD_LEN, isSubOSD_Filled);
            packetOSD_SizeSub = body_len + HEAD_LEN;

        } break;

        case G_APP_COMMAND: {  // NOT USE IT NOW.  it's only for the Main
            int msg_id = rx_head[IDX_MSG_ID];
            int size_100 = rx_head[IDX_LENGTH_100];  // OSD LENGTH / 100 (SubHeader + Body, No Tail)
            int size_1 = rx_head[IDX_LENGTH_1];      // OSD LENGTH % 100 (SubHeader + Body, No Tail)
            int body_len = size_100 * 100 + size_1;

            currentRxBuffer = packetBuf;

            read_len = uart_read_bytes(UART_NUM_0, currentRxBuffer, body_len, portMAX_DELAY);

/*            
            onWifiSetting = true;
            int ret_val = parsePacket_WifiApConfigInfo(rx_head, currentRxBuffer, body_len);
            if(ret_val < 0) {
                uart0_printf("#[%8d]>> ERROR, parsePacket_WifiApConfigInfo failed with %d \n", millis(), ret_val);
                buildPacket_WifiApConfigAck(currentRxBuffer, 0);  // Send FAIL ack
            } else {
                uart0_printf("#[%8d]>> parsePacket_WifiApConfigInfo success with %d \n", millis(), ret_val);
                buildPacket_WifiApConfigAck(currentRxBuffer, 1);  // Send SUCCESS ack
            }
            sendPacket0(currentRxBuffer, HEAD_LEN + 2);  // Send ack

            onWifiSetting = false;  // Reset wifi setting flag
*/
        }break;

        case G_DEVICE_IO:      // no RX on uart0
        case G_SENSOR_DATA: {  // no RX on uart0
            return -10;
        } break;

        default:
            uart0_printf("#[%8d]>> ERROR, WRONG GROUP ID = %d \n", rx_GROUP_ID);
            return -2;
    }

    // uart0_println("Packet received successfully");

    return 0;
}

int processRX1(byte *rx1_packet, int rx1_packet_size) {
    uart_flush_input(UART_NUM_1);  // RX 버퍼 memset

    byte *rx_head = rx1_packet;
    byte *bufferToParse = rx1_packet + HEAD_LEN;

    int read_len = rx1_packet_size;

    if (checkPacketHead(rx_head) == false) {
        // uart0_printf("[%8d] RX1 Header BAD- size = %d/%d (ME = %d) \n", millis(), read_len, rx1_packet_size, MY_BOARD_ID);
        // printPacketHeader(rx_head, 10);

        return -1;
    }

    int rx_TX_BOARD_ID = rx_head[IDX_TX_BOARD_ID];
    int rx_GROUP_ID = rx_head[IDX_GROUP_ID];
    int rx_MSG_ID = rx_head[IDX_MSG_ID];

    switch (rx_GROUP_ID) {
        case G_DEVICE_IO: {  // main : only tx,  sub : only rx
            switch (rx_MSG_ID) {
                case M_PERMIT:
                    parsePacket_Permit(rx_head, nullptr, MY_BOARD_ID, isPermitGranted);
                    // uart0_printf("[%8d] RX1 PERMIT (%d to %d)  [2]%3d \n", millis(),
                    //              rx_TX_BOARD_ID, rx_head[IDX_PERMIT_ID], rx_head[IDX_VER]);
                    uart0_printf("Pp"); // read data

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
            uart0_printf("D%d", tx_board_id); // read data

            // uart0_printf("\n[%8d] RX1, Sensor {Sender:%d ==> ME} [2]%d \n",
            //              millis(), tx_board_id, rx_head[IDX_VER]);

            if (BoardID_MAX == tx_board_id) {
                // isBoard0_duringPermitCycle = false;

                // uart0_printf("[%8d] RX1 Sensor completed = %d, [2]=%d \n\n", millis(), tx_board_id, rx_head[2]);
            }

            return 0;
        } break;

        case G_OSD_COMMAND: {  // in case 485, G_OSD_COMMAND is only for the sub
            // bufferToParse = packetBuf_OSD;

            //  PARSE OSD DATA
            // parsePacket_OSD_bySub(MY_BOARD_ID, rx_head, bufferToParse);

        } break;

        case G_APP_COMMAND:  // NOT USE IT NOW. main : only tx,  sub : only rx
            break;

        default:
            uart0_printf("[%8d]>> ERROR, WRONG GROUP ID = %d \n", rx_GROUP_ID);
            return -2;
    }

    return 0;
}

SemaphoreHandle_t semaSendPermit;  // 세마포어 핸들 생성

void considerToPermit(int permit_interval) { // 추후 사용 가능성 있음.
    if (MY_BOARD_ID != 0)
        return;

    // permit_interval is 1 or 120
    if ((millis() - tickPermitLast) < permit_interval) {
        // if (isSubOSD_Filled == true)
        //     continue;
        return;
    }

    if (xSemaphoreTake(semaSendPermit, portMAX_DELAY) == pdTRUE) {
        reser_1 = (reser_1 + 1) % 200;
        buildPacket_Permit(packetBuf_DeviceIO, M_BOARD_1);  // M_BOARD_1 : destination board id

        // isBoard0_duringPermitCycle = true;
        sendPacket1(packetBuf_DeviceIO, HEAD_LEN + TAIL_LEN);

        tickPermitLast = millis();

        // uart0_printf("[%8d] send permit{%d} dur%d ms (last=%d) [7]%d \n", millis(), permit_interval, millis() - tickPermitLast,
        //             tickPermitLast, reser_1);
        xSemaphoreGive(semaSendPermit);
    }
}

void osdSimulation() {
    int osd_packet_sub_len = 16 + 2;
    byte osd_packet_buffer[PACKET_LEN_OSD];
    if(pumpCount % 1 == 0){
        int start_x = 0 + ((pumpCount + 13) % 16);
        // int start_x = 2;
        // int start_y = 0 + pumpCount % 20;
        int start_y = 0;
        int width = 4 + ((pumpCount % 2) * 6);
        int height = 35;
        osd_packet_sub_len = 16 + width * height + 2;

        memset(osd_packet_buffer, 0xef, PACKET_LEN_OSD);

        buildPacket_OSD_1Bd(MY_BOARD_ID, osd_packet_buffer, start_x, start_y, width, height);
        // copyPacketToOSDBuf(MY_BOARD_ID, osd_packet_buffer, (osd_packet_buffer + HEAD_LEN));
        // isSubOSD_Filled = true;
    }
/*
    if (isSubOSD_Filled == true) {
        // osd_packet_buffer[9] = 0; // 9: start y
        sendPacket1(osd_packet_buffer, osd_packet_sub_len);
        isSubOSD_Filled = false;
        // printUart1("#OSD test packet %d.\n", osd_packet_sub_len);
    }
*/
}

//  main task on core 0
void pumpSerial(void *pParam) {
    uart0_printf("[%8d]PUMP- ENTER\n", millis());
    // esp_task_wdt_delete(NULL);  // 현재 태스크를 WDT 감시에서 제외

    int ret_val = 0;

    while (true) {
        vTaskDelay(1); // pumpSerial() task 초입.

        // if(isOTACommand == true)
        //     continue;

        // uart0_printf("[%8d] PUMP loop = %d (dip=%d) \n", millis(), pumpCount, MY_BOARD_ID);

        if (MY_BOARD_ID == 0) {
            if (isSensorDataFilled == false) {
                continue;
            }
            // if (isBoard0_duringPermitCycle == true) {
            //     continue;
            // }

            // Send OSD Buffer - Main, Sub
            // osdSimulation();
            // if (isSubOSD_Filled == true) {
            //     sendPacket1(packetBuf_OSDSub, packetOSD_SizeSub);
            //     isSubOSD_Filled = false;
            //     printUart1("#OSD board = %d, Size=%d\n", packetBuf_OSDSub[IDX_MSG_ID] & 0x0F, packetOSD_SizeSub);
            // }

            // Send Sensor Data to UART0
            // buildPacket_Sensor_1Set(MY_BOARD_ID, packetBuf, forceBuffer_rd, NUM_1SET_SEN_WIDTH, NUM_1SET_SEN_HEIGHT);
            if(dip_state[4] == HIGH) { // Serial
                // sendPacket0(packetBuf, PACKET_LEN_SEN_1SET);  // it consumes 11ms per one board. so for 2, it consumes 22ms.
            }
            else { // TCP
                // loop_tcpStar();
                // sendPacketTCP(packetBuf, PACKET_LEN_SEN_1SET);
                // buildAndSendPacket_Test();
            }

            // Send Permit to Sub#1
            // considerToPermit(50);  //  1 : 1ms interval

            isSensorDataFilled = false;  // turn on loopADCRead, it will give sema to draw led task.

        } else {
            // if (isSensorDataFilled == false) {
            //     continue;
            // }
            // if (isPermitGranted == false) {
            //     continue;
            // }
            
            // buildPacket_Sensor_1Bd(MY_BOARD_ID, packetBuf, forceBuffer_rd, SIZE_X, SIZE_Y);
            // sendPacket1(packetBuf, PACKET_LEN_SEN_1Bd);

            if(BoardID_MAX == 7) {
                // packetBuf[3] = 2;
                // sendPacket1(packetBuf, PACKET_LEN_SEN_1Bd);
                // packetBuf[3] = 3;
                // sendPacket1(packetBuf, PACKET_LEN_SEN_1Bd);
                // packetBuf[3] = 4;
                // sendPacket1(packetBuf, PACKET_LEN_SEN_1Bd);
                // packetBuf[3] = 5;
                // sendPacket1(packetBuf, PACKET_LEN_SEN_1Bd);
                // packetBuf[3] = 6;
                // sendPacket1(packetBuf, PACKET_LEN_SEN_1Bd);
                // packetBuf[3] = 7;
                // sendPacket1(packetBuf, PACKET_LEN_SEN_1Bd);
            }

            isSensorDataFilled = false;
            isPermitGranted = false;
        }

        pumpCount++;
    }  // while

    return;
}

#endif  // _COMMPACKET_H_