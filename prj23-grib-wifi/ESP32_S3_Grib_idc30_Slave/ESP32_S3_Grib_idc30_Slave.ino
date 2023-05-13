/*
  <Arduino Config>a
  Boards Manager download : ESP32 by Espressif Systems.
  BOARD : "ESP32-S3 DEVKITC-1" (NOT 1.1) OR "ADAFRUIT QT py esp32-C3"
  DESC : SRAM 512K, ROM 384K, FLASH 8M, PSRAM 2M
  CONFIG : Flash QIO 80MHz, OPI PSRAM, Partition Minimal SPIFFS, others : apply default setting
  URL : https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1-v1.0.html

  IF using BPI-LEAF-S3 board,
    for the configuration, visit : https://github.com/BPI-STEAM/BPI-Leaf-S3-Doc/tree/main/Example/Arduino
    Firstly click and hold down boot button and clock rst and then release boot button.
    And after uploading, you should click rst button..
*/

#include "commPacket.h"
#include "loop_WifiClient.h"

TaskHandle_t Task_Core0;
#define CORE_0 0
#define CORE_1 1

unsigned long myTime;

unsigned long timeStamp0 = 0;
void startTS0() { timeStamp0 = millis(); }
unsigned long getElapsed_TS0() { return (millis() - timeStamp0); }

unsigned long timeStamp1;
void startTS1() { timeStamp1 = millis(); }
unsigned long getElapsed_TS1() { return (millis() - timeStamp1); }

boolean adc_scan_done = false;

void setup()
{
  Serial.begin(BAUD_RATE);

  Serial1.setTxBufferSize(SERIAL_SIZE_TX);
  Serial1.begin(BAUD_RATE, SERIAL_8N1, 18, 17);

  memset(packetBuf, 0, PACKET_LEN);

  setup_HWPins_Grib();
  selectMux(0);

  set_WIFIClient(); //-------------   WIFI    WIFI    WIFI

  //------------------------------
  //  TASKS
  xTaskCreatePinnedToCore(
      pumpSerial,             // 태스크 함수
      "pumpSerial",           // 테스크 이름
      2048,                   // 스택 크기(워드단위)
      (void *)&adc_scan_done, // 태스크 파라미터
      100,                    // 태스크 우선순위
      &Task_Core0,            // 태스크 핸들
      CORE_0);                // 실행될 코어

  startTS1();
}

int loop_count = 0;
int adc_scan_count = 0;
void loop()
{
  //  CHANGE MUX AND READ ADC 16EA
  if (adc_scan_done == false)
  {
    // Serial.printf("[time1:%08d] ADC start(%d) %d \n", millis(), adc_scan_done, getElapsed_TS1()); // prints time since program started

    for (int mux_id = 0; mux_id < MUX_LIST_LEN; mux_id++)
    {
      changeMux(mux_id);
      read_16ch_in_mux_fast(adc_value[mux_id]);
    }
    adc_scan_count++;

    adc_scan_done = true;

    // Serial.printf("[time1:%08d] ADC end__(%d:%d) %d \n", millis(), adc_scan_done, adc_scan_count, getElapsed_TS1()); // prints time since program started
  }

  loop_count++;
}

void pumpSerial(void *pParam)
{
  boolean core1_done = *((boolean *)(pParam));

  while (true)
  {
    if (adc_scan_done == false)
    {
      // Serial.printf("[time0:%08d] delay (%d) %d \n", millis(), adc_scan_done, getElapsed_TS0()); // prints time since program started
      delay(1);
      continue;
    }

    Serial.printf("[time0:%08d] tx start(%d) %d \n", millis(), adc_scan_done, getElapsed_TS0()); // prints time since program started
    startTS0();

    //  SEND PACKET
    // buildPacket(packetBuf, adc_value, MUX_LIST_LEN, NUM_MUX_OUT);
    // sendPacket(packetBuf, PACKET_LEN);

    int data_width = PARCEL_MUX_LEN;
    int data_height = NUM_MUX_OUT; // 16

    int start_index = 0;

    for (int col = 0; col < MUX_LIST_LEN;)
    {
      buildParcel(packetBuf, adc_value, data_width, data_height, col, col + data_width);
      sendSerial(packetBuf, PARCEL_LEN);

      col += data_width;

      delay(5);
    }

    Serial.printf("[time0:%08d] tx end__(%d)   %d \n", millis(), adc_scan_done, getElapsed_TS0()); // prints time since program started

    adc_scan_done = false;
  } // while
}

void sendSerial(byte *packet_buffer, int packet_len)
{
  Serial1.write(packet_buffer, packet_len);
  /*
    Serial.printf("\nloop=%dof %d [s=%d, l=%d]\n", loop_count, packet_len, packet_buffer[0], packet_buffer[packet_len-1]);

    for(int mux_id = (6 - 1) ; 0 <= mux_id ; mux_id--) {
      Serial.printf("[mux:%2d] ", mux_id);

      for (int i = 0 ; i < NUM_MUX_OUT ; i++) {
        Serial.printf("%3d, ", packet_buffer[HEADER_LEN + mux_id * NUM_MUX_OUT + i]);
      }
      Serial.println("~");
    }
  */
}
