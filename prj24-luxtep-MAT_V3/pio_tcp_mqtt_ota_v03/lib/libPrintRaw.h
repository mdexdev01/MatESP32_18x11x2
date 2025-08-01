#ifndef _LIB_PRINT_RAW_H_
#define _LIB_PRINT_RAW_H_

#include "driver/uart.h"
#include <stdarg.h> // va_list, va_start, va_end, vsnprintf 사용을 위해 추가

#define BAUD_RATE0 921600 // 요청하신 보드레이트 정의

// UART0 초기화 함수 (한 번만 호출되어야 함)
void init_uart0() {
    uart_config_t uart_config_0 = {
        .baud_rate = BAUD_RATE0,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };
    uart_param_config(UART_NUM_0, &uart_config_0);
    uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // UART 드라이버 설치 (RX 버퍼 크기, TX 버퍼 크기, 이벤트 큐, 이벤트 큐 크기, 이벤트 큐 핸들, 인터럽트 플래그)
    uart_driver_install(UART_NUM_0, 256, 0, 0, NULL, 0); 
}

// UART0에서 printf 스타일 출력 함수
void uart0_printf(const char *fmt, ...) {
    char buffer[128];
    va_list args;

    // 가변 인자 처리
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    // UART0으로 데이터 송신
    uart_write_bytes(UART_NUM_0, buffer, strlen(buffer));
}

#define PRINT_BUF_SIZE 256  // 출력 버퍼 크기

// 기본 문자열 출력 함수
void uart0_println(const char *fmt, ...) {
    char buffer[PRINT_BUF_SIZE];  // 출력 버퍼
    va_list args;

    // 가변 인자 처리
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    // 문자열 출력
    uart_write_bytes(UART_NUM_0, buffer, strlen(buffer));
    // 개행 추가
    uart_write_bytes(UART_NUM_0, "\n", 1);
}

// 정수 처리
void uart0_println(int value) {
    char buffer[PRINT_BUF_SIZE];
    snprintf(buffer, sizeof(buffer), "%d", value);
    uart0_println(buffer);
}

// String 객체 처리
void uart0_println(const String &value) {
    uart0_println(value.c_str());
}

void uart0_print(const char *fmt, ...) {
    char buffer[128];
    va_list args;

    // 가변 인자 처리
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    // UART0으로 데이터 송신
    uart_write_bytes(UART_NUM_0, buffer, strlen(buffer));
}

void uart1_printf(const char *fmt, ...) {
    char buffer[256];
    va_list args;

    // 가변 인자 처리
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    // UART1으로 데이터 송신
    uart_write_bytes(UART_NUM_1, buffer, strlen(buffer));
}

#endif  //_LIB_PRINT_RAW_H_