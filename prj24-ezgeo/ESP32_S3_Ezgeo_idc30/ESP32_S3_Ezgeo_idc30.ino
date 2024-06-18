/*
  ESP32 S3 Bed sensor (WinnieConnie, Smartmat)
*/

/*
  <Arduino Config>
  Boards Manager download : ESP32 by Espressif Systems.
  BOARD : "ESP32-S3 DEVKITC-1" (NOT 1.1) OR "ADAFRUIT QT py esp32-C3"
  DESC : SRAM 512K, ROM 384K, FLASH 8M, PSRAM 2M
  CONFIG : Flash QIO 80MHz, OPI PSRAM, Partition Minimal SPIFFS, others : apply default setting
  URL : https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1-v1.0.html

  < Reference >
  MultiTask : https://blog.naver.com/PostView.naver?blogId=crucian2k3&logNo=222069416341&parentCategoryNo=&categoryNo=58&viewDate=&isShowPopularPosts=true&from=search

  < TODO >
  1. 점퍼 번호가 2씩 밀린다. 즉 HW 0번을 묶으면 SW에서는 2번 먹스로 읽힌다. 종성이가 2개를 앞에서부터 자른것 같다.

*/

#include "commPacket.h"
#include "lib_ble_grib.h"
#include "lib_gpio.h"
// #include "lib_wdtimer.h"

TaskHandle_t Task_Core0;
#define CORE_0 0
#define CORE_1 1

void setup() {
    Serial.setTxBufferSize(SERIAL_SIZE_TX);
    Serial.begin(BAUD_RATE);

    Serial1.setRxBufferSize(SERIAL_SIZE_RX);
    Serial1.begin(BAUD_RATE, SERIAL_8N1, 18, 17);

    memset(packetBuf, 0, PACKET_LEN);

    // Serial.printf("SETUP-GPIO \n");
    // setup_gpioWork();

    Serial.printf("SETUP-HW PINS \n");
    setup_HWPins_Grib();
    selectMux(0);

    Serial.printf("SETUP-WD TIMER \n");
    setup_wdTimer();

    //------------------------------
    //  TASKS - 00
    Serial.printf("TASK CREATES \n");
    xTaskCreatePinnedToCore(
        pumpSerial,    // 태스크 함수
        "pumpSerial",  // 테스크 이름
        (1024 * 10),   // 스택 크기(워드단위)
        NULL,          // 태스크 파라미터
        100,           // 태스크 우선순위
        &Task_Core0,   // 태스크 핸들
        CORE_0);       // 실행될 코어
}

int adc_scan_done = false;

int loop_count = 0;
void loop() {
    // Serial.printf("\nLOOP Count2 = %d \n", loop_count);
    if (adc_scan_done == false) {
        adcScanMainPage();
    } else {
        delay(1);
    }

    loop_wdTimer();
    loop_gpioWork();
    loop_advGrib();

    loop_count++;
}

int GET_1of2(int num) { return (num % 2); }

int adc_scan_count_main = 0;
void adcScanMainPage() {
    //  CHANGE MUX AND READ ADC 16EA
    for (int mux_id = 0; mux_id < MUX_LIST_LEN; mux_id++) {
        changeMux(mux_id);
        // read_16ch_in_mux_fast(adc_value[mux_id]);  //  adc_value : 2d array
        read_8ch_in_mux(adc_value[mux_id]);  //  adc_value : 2d array
        reorder_bug_patch(adc_value[mux_id]);  //  adc_value : 2d array
    }

    adc_scan_done = true;
    adc_scan_count_main++;
}

int deliver_count_main = 0;

void pumpSerial(void *pParam) {
  Serial.printf("PUMP- ENTER\n");
  while (true) {
      if (adc_scan_done == false) {
          delay(1);
          continue;
      }

      //  SENSOR DATA - MAIN
      buildPacket(packetBuf, adc_value, MUX_LIST_LEN, NUM_USED_OUT);

      sendPacket(packetBuf, PACKET_LEN);

      adc_scan_done = false;
      // tempDelay(1000);

      deliver_count_main++;
  }  // while
}

void sendPacket(byte *packet_buffer, int packet_len) {
    //  send data to the PC
    Serial.write(packet_buffer, packet_len);

    //  log all data.
    // printPacket(packet_buffer, packet_len);
}

void printHexa(byte *hex_data, int hex_len) {
    for (int i = 0; i < hex_len; i++) {
        Serial.printf("%02x", hex_data[i]);
    }
}

void printPacket(byte *packet_buffer, int packet_len) {
    //  log all data.
    int offset = 0;
    Serial.printf("loop=%d [s=%d, l=%d]\n", loop_count, packet_buffer[offset], packet_buffer[packet_len - 1]);
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

    int matrix_num_unit = packet_buffer[5];
    int matrix_unit_len = packet_buffer[6];
    int body_len = matrix_num_unit * matrix_unit_len;

    Serial.printf(" adc data length : %d \n", body_len);

    for (int i = 0; i < body_len; i++) {
        Serial.printf("%3d,", packet_buffer[HEADER_LEN + i]);
        if (((i + 1) % NUM_USED_OUT) == 0)
            Serial.println("~");
    }

    offset += body_len;

    Serial.printf("[%d] %3d ", offset, packet_buffer[offset]);
    offset++;
    Serial.printf("[%d] %3d  \n", offset, packet_buffer[offset]);
    offset++;
}
