#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define TX_PIN 17             // UART TX 핀
#define RX_PIN 18             // UART RX 핀
#define DE_PIN 19             // RS485 DE 핀

#define BAUD_RATE1 2000000    // UART1 보드레이트
#define BUF_SIZE 1024         // RX 버퍼 크기

#define PACKET_HEAD_LEN 8
#define PACKET_TAIL_LEN 2
#define SUB_HEAD_LEN 8
#define NUM_SUB_WIDTH 28
#define NUM_SUB_HEIGHT 34
#define OSD_BUFFER_SIZE (NUM_SUB_WIDTH * (NUM_SUB_HEIGHT + 1))
#define OSD_PACKET_SIZE (PACKET_HEAD_LEN + SUB_HEAD_LEN + OSD_BUFFER_SIZE + PACKET_TAIL_LEN)

byte packetBufOSD_rx1[OSD_PACKET_SIZE];  // Double Buffer 1
byte packetBufOSD_rx2[OSD_PACKET_SIZE];  // Double Buffer 2
byte* currentRxBuffer;                   // 현재 수신 중인 버퍼

QueueHandle_t uart_queue;

void setDE(bool state) {
    if (state) {
        digitalWrite(DE_PIN, HIGH);
        delayMicroseconds(800); // 안정화 지연
    } else {
        delayMicroseconds(800);
        digitalWrite(DE_PIN, LOW);
    }
}

// 데이터 수신 및 3단계 처리 함수
void processPacket() {
  uart_flush_input(UART_NUM_1);
    // int headerLen = Serial1.readBytes(currentRxBuffer, PACKET_HEAD_LEN);
    int headerLen = uart_read_bytes(UART_NUM_1, currentRxBuffer, PACKET_HEAD_LEN, portMAX_DELAY);

    if (currentRxBuffer[0] != 0xFF || currentRxBuffer[1] != 0xFF) {
        Serial.printf("Header Error:{%d}, %d, %d, %d, %d, %d, %d, %d, %d \n", headerLen, 
                currentRxBuffer[0], currentRxBuffer[1], currentRxBuffer[2], currentRxBuffer[3], 
                currentRxBuffer[4], currentRxBuffer[5], currentRxBuffer[6], currentRxBuffer[7]);
        uart_flush_input(UART_NUM_1);
        return;
    }
    Serial.printf("Header :{%d}, %d, %d, %d, %d, %d, %d, %d, %d \n", headerLen, 
            currentRxBuffer[0], currentRxBuffer[1], currentRxBuffer[2], currentRxBuffer[3], 
            currentRxBuffer[4], currentRxBuffer[5], currentRxBuffer[6], currentRxBuffer[7]);

    // 2단계: 서브 헤더 읽기
    // int subHeaderLen = Serial1.readBytes(currentRxBuffer + PACKET_HEAD_LEN, SUB_HEAD_LEN);
    int subHeaderLen = uart_read_bytes(UART_NUM_1, currentRxBuffer + PACKET_HEAD_LEN, SUB_HEAD_LEN, portMAX_DELAY);
    if (subHeaderLen != SUB_HEAD_LEN) {
        Serial.println("Sub-header Error");
        uart_flush_input(UART_NUM_1);
        return;
    }

    Serial.printf("Sub Header :{%d}, %d, %d, %d, %d, %d, %d, %d, %d \n", headerLen, 
            currentRxBuffer[8], currentRxBuffer[9], currentRxBuffer[10], currentRxBuffer[11], 
            currentRxBuffer[12], currentRxBuffer[13], currentRxBuffer[14], currentRxBuffer[15]);

    int x_size = currentRxBuffer[10];
    int y_size = currentRxBuffer[11];
    int bodyLen = x_size * y_size;

    if (bodyLen <= 0 || bodyLen > OSD_BUFFER_SIZE) {
        Serial.printf("body x = %d , y = %d \n", x_size, y_size);
        return;
    }

    // 3단계: 본문 및 테일 읽기
    // int bodyTailLen = Serial1.readBytes(currentRxBuffer + PACKET_HEAD_LEN + SUB_HEAD_LEN, bodyLen + PACKET_TAIL_LEN);
    int bodyTailLen = uart_read_bytes(UART_NUM_1, currentRxBuffer + PACKET_HEAD_LEN + SUB_HEAD_LEN, 2, portMAX_DELAY);
    if (bodyTailLen != bodyLen + PACKET_TAIL_LEN) {
        Serial.println("Body and Tail Error");
        return;
    }

    Serial.println("Packet received successfully");
}

void uart_event_task(void* pvParameters) {
    uart_event_t event;
    while (true) {
        if (xQueueReceive(uart_queue, (void*)&event, portMAX_DELAY)) {
            switch (event.type) {
                case UART_DATA:
                    processPacket(); // 패킷 처리
                    break;

                case UART_FIFO_OVF:
                    Serial.println("RX FIFO Overflow!");
                    uart_flush_input(UART_NUM_1);
                    break;

                case UART_BUFFER_FULL:
                    Serial.println("RX Buffer Full!");
                    uart_flush_input(UART_NUM_1);
                    break;

                default:
                    break;
            }
        }
    }
}

void sendData(byte* data, size_t length) {
    setDE(true); // 송신 모드 전환
    uart_write_bytes(UART_NUM_1, data, length);
    uart_wait_tx_done(UART_NUM_1, 100 / portTICK_PERIOD_MS); // 송신 완료 대기
    setDE(false); // 수신 모드 전환
}

void setup() {
    Serial.begin(921600);

    // DE 핀 설정
    pinMode(DE_PIN, OUTPUT);
    setDE(false); // 초기값: 수신 모드

    // UART 설정
    uart_config_t uart_config = {
        .baud_rate = BAUD_RATE1,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 20, &uart_queue, 0);

    // Double Buffer 초기 설정
    currentRxBuffer = packetBufOSD_rx1;

    // UART 이벤트 처리 태스크 생성
    xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 10, NULL);
}

int loop_cnt = 0;
void loop() {
    byte message [20];
    message[0] = 0xff;
    message[1] = 0xff;
    message[2] = 2;
    message[3] = 3;
    message[4] = 4;
    message[5] = 5;
    message[6] = 6;
    message[7] = 7;
    message[8] = 8;
    message[9] = 9;
    memset(message + 10, 50, 10);

    message[15] = loop_cnt % 100 + 100;

    sendData(message, sizeof(message));
  
  delayMicroseconds(1000);
  loop_cnt++;
}
