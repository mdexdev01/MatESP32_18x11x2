/*
  ESP32 S3 floor sensor (BrandContents, Smartmat)
*/

/*
  <Arduino Config>
  Boards Manager download : ESP32 by Espressif Systems.
  BOARD : "ESP32-S3 DEVKITC-1" (NOT 1.1) OR "ADAFRUIT QT py esp32-C3"
  DESC : SRAM 512K, ROM 384K, FLASH 8M, PSRAM 2M
  CONFIG : Flash QIO 80MHz, OPI PSRAM, Partition Minimal SPIFFS, others : apply default setting
  URL : https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1-v1.0.html
  Board Manager : Arduino ESP32 Boards Ver.2.0.13 (Critical for the timer)

  < Reference >
  MultiTask : https://blog.naver.com/PostView.naver?blogId=crucian2k3&logNo=222069416341&parentCategoryNo=&categoryNo=58&viewDate=&isShowPopularPosts=true&from=search

  < TODO >
  1. 시간 간격 축소. 1초당 40회
  2. 하중으로 변환

*/

#include "commPacket.h"
#include "lib_ble_grib.h"
#include "lib_gpio.h"
// #include "lib_wdtimer.h"

TaskHandle_t Task_Core0;
#define CORE_0 0
#define CORE_1 1

bool stringComplete = false;  // whether the string is complete
String inputString = "";      // a String to hold incoming data

void setup() {
  inputString.reserve(200);

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
  
  afterSetup();
}

void afterSetup(){
  //------------------------------
  //  Check Dip switch
  adcScanMainPage();
  updateDipSwVal();
}

/*
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    Serial.println(inChar);
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }

  if (stringComplete) {
    Serial.println(inputString);
    // clear the string:
    inputString = "";
    stringComplete = false;
  }
}
*/

int adc_scan_done = false;

int loop_count = 0;
void loop() {
  // Serial.printf("\nLOOP Count2 = %d \n", loop_count);
  if (adc_scan_done == false) {
      adcScanMainPage();
      reorder_sensor_shape();
      { //  RS485 setting
        updateDipSwVal();
        if(dip_decimal == 0) {
          //  main unit - UART1 : RX
          digitalWrite(pin485U1DE, LOW); // LOW : RX, HIGH : TX
        }
        else {
          //  sub unit - UART1 : Tx
          digitalWrite(pin485U1DE, HIGH); // LOW : RX, HIGH : TX
        }
      }
  } else {
      delay(1);
  }

  loop_wdTimer();
  loop_gpioWork();
  // loop_advGrib();

  loop_count++;
}

int GET_1of2(int num) { return (num % 2); }

int adc_scan_count_main = 0;
void adcScanMainPage() {
  //  CHANGE MUX AND READ ADC 16EA
  for (int mux_id = 0; mux_id < MUX_LIST_LEN; mux_id++) {
    changeMux(mux_id);
    read_16ch_in_mux_fast(adc_value[mux_id]);  //  adc_value : 2d array
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

    //  SENSOR DATA - SLAVE
    //  true : deliver master + slave. false : deliver only master
    //  if partial packet mode, it delivers all the partial packets of a single full packet in a while loop.
    if(dip_decimal == 0) { // Main board
      while (true) {
        int ret_val = deliverSlaveUart();  // enable only for the Master

        if (ret_val <= 0)
            break;
      }
    }

    //  SENSOR DATA - MAIN
    // buildPacket(packetBuf, adc_value, MUX_LIST_LEN, NUM_USED_OUT);
    buildPacket_brandContents(packetBuf, adc_value, MUX_LIST_LEN, NUM_USED_OUT);

    if(timer_flag == true) {
      // ets_printf("timer on  %d, (%d) \n", timer_count, millis());

      if(dip_decimal == 0) {
        sendPacket0(packetBuf, PACKET_LEN);
      }
      else {
        sendPacket1(packetBuf, PACKET_LEN);
      }
      timer_flag = false;
    }


    adc_scan_done = false;
    tempDelay(1);

    deliver_count_main++;
  }  // while
}


//  send data to the PC
void sendPacket0(byte *packet_buffer, int packet_len) {
  Serial.write(packet_buffer, packet_len);

  //  log all data.
  // printPacket(packet_buffer, packet_len);
}


//  send data to the PC
void sendPacket1(byte *packet_buffer, int packet_len) {
  digitalWrite(pin485U1DE, HIGH); // LOW : RX, HIGH : TX
  // Serial1.printf("Hello \n");
  
  Serial1.write(packet_buffer, packet_len);
  // Serial.write(packet_buffer, packet_len);

  //  log all data.
  printPacket(packet_buffer, packet_len);
}


