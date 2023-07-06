#include <bluefruit.h>

/*
  <Arduino Config>
  Boards Manager download : ESP32 by Espressif Systems.
  BOARD : "ESP32-S3 DEVKITC-1" (NOT 1.1) OR "ADAFRUIT QT py esp32-C3"
  DESC : SRAM 512K, ROM 384K, FLASH 8M, PSRAM 2M  
  CONFIG : Flash QIO 80MHz, OPI PSRAM, Partition Minimal SPIFFS, others : apply default setting
  URL : https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1-v1.0.html
*/

/*
  < Reference >
  MultiTask : https://blog.naver.com/PostView.naver?blogId=crucian2k3&logNo=222069416341&parentCategoryNo=&categoryNo=58&viewDate=&isShowPopularPosts=true&from=search
*/

#include "commPacket.h"

TaskHandle_t Task_Core0;
#define CORE_0    0
#define CORE_1    1


void setup() {
  Serial.setTxBufferSize(SERIAL_SIZE_TX);
  Serial.begin(BAUD_RATE);

  Serial1.setRxBufferSize(SERIAL_SIZE_RX);
  Serial1.begin(BAUD_RATE, SERIAL_8N1, 18, 17);

  memset(packetBuf, 0, PACKET_LEN);
  
  setup_HWPins_Grib();
  selectMux(0);

  //------------------------------
  //  TASKS  
    xTaskCreatePinnedToCore(
    pumpSerial,       // 태스크 함수
    "pumpSerial",     // 테스크 이름
    4096,             // 스택 크기(워드단위)
    NULL,             // 태스크 파라미터
    100,              // 태스크 우선순위
    &Task_Core0,      // 태스크 핸들
    CORE_0);               // 실행될 코어

}

int adc_scan_done = false;

int loop_count = 0;
void loop() {
  // Serial.printf("loop_count2 = %d \n", loop_count);
  if(adc_scan_done == false) {
    //  CHANGE MUX AND READ ADC 16EA
    for(int mux_id = 0 ; mux_id < MUX_LIST_LEN ; mux_id++) {
      changeMux(mux_id);
      read_16ch_in_mux_fast(adc_value[mux_id]);
    }

    adc_scan_done = true;
  }
  else {
    delay(1);
  }


  loop_count++;
}



void pumpSerial(void * pParam) {  
  while(true) {
    if(adc_scan_done == false) {
      delay(1);
      continue;
    }

    //  SENSOR DATA - SLAVE
    while(true) {
      int ret_val = deliverSlaveUart(); // enable only for the Master

      if(ret_val < 0)
        break;      
    }

    //  SEND SERIAL
    if((loop_count % 1) == 0) {
      buildPacket(packetBuf, adc_value, MUX_LIST_LEN, NUM_MUX_OUT);
      sendSerial(packetBuf, PACKET_LEN);
    }
    adc_scan_done = false;

  } // while
  
}


void sendSerial(byte * packet_buffer, int packet_len) {
  Serial.write(packet_buffer, packet_len);

/*
  Serial.printf("\nloop=%d [s=%d, l=%d]\n", loop_count, packet_buffer[0], packet_buffer[packet_len-1]);

  for(int mux_id = (MUX_LIST_LEN - 1) ; 0 <= mux_id ; mux_id--) {
    Serial.printf("[mux:%2d] ", mux_id);

    for (int i = 0 ; i < NUM_MUX_OUT ; i++) {
      Serial.printf("%3d, ", packet_buffer[HEADER_LEN + mux_id * NUM_MUX_OUT + i]);
    }
    Serial.println("~");
  }
*/
}

