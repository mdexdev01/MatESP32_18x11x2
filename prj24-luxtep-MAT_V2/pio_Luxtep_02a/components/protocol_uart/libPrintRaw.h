#ifndef _LIB_PRINT_RAW_H_
#define _LIB_PRINT_RAW_H_

#include "driver/uart.h"

// #define ESP_LOGM(tag, fmt, ...)  printf("[%s] " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGM(tag, fmt, ...)  printf(fmt, ##__VA_ARGS__)

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