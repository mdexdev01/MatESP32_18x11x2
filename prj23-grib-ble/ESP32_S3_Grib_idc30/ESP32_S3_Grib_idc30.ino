/*
  ESP32 S3 BLE Server (Grib, Smartmat)
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

*/

#include "commPacket.h"
#include "lib_ble_grib.h"

TaskHandle_t Task_Core0;
#define CORE_0 0
#define CORE_1 1

void setup() {
    Serial.setTxBufferSize(SERIAL_SIZE_TX);
    Serial.begin(BAUD_RATE);

    Serial1.setRxBufferSize(SERIAL_SIZE_RX);
    Serial1.begin(BAUD_RATE, SERIAL_8N1, 18, 17);

    memset(packetBuf, 0, PACKET_LEN);

    Serial.printf("SETUP-HW PINS \n");
    setup_HWPins_Grib();
    selectMux(0);

    Serial.printf("GRIB-advertising \n");
    setup_advGrib();
    delay(10);

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

        adc_scan_done = true;
    } else {
        // buildPacket(packetBuf, adc_value, MUX_LIST_LEN, NUM_MUX_OUT);

        // loop_advGrib(packetBuf, PACKET_LEN);
        // sendPacket(packetBuf, PACKET_LEN);

        delay(1);
    }
    
    loop_advGrib();
    loop_count++;
}

int GET_1of2(int num) { return (num % 2); }

int adc_scan_count_main = 0;
void adcScanMainPage() {
    // Serial.printf("[%d] ADC Full scan - main\n", adc_scan_count_main);
    //  CHANGE MUX AND READ ADC 16EA
    for (int mux_id = 0; mux_id < MUX_LIST_LEN; mux_id++) {
        changeMux(mux_id);
        read_16ch_in_mux_fast(adc_value[mux_id]);  //  adc_value : 2d array
    }

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
        while (true) {
          int ret_val = deliverSlaveUart();  // enable only for the Master

          if (ret_val <= 0)
              break;
        }

        //  SENSOR DATA - MAIN
        buildPacket(packetBuf, adc_value, MUX_LIST_LEN, NUM_MUX_OUT);
        {
          int packet_rle_size = 0;
          if( false == encodePacketToRle(packetBuf, PACKET_LEN, packetBufEnc, PACKET_ENC_LEN, packet_rle_size) ) {
            Serial.printf("encoding fail \n");
          }

          int packet_dec_size = 0;
          if( false == decodePacketToRle(packetBufEnc, packetBufDec, packet_dec_size) ) {
            Serial.printf("decoding fail \n");
          }

          if( false == is_same_buf(packetBuf, packetBufDec, PACKET_LEN) ) {
            Serial.printf("\n2 Codec error !! enc_size=%d, dec_size=%d \n", packet_rle_size, packet_dec_size);

            // Serial.printf("-- original buf \n");
            // printPacket(packetBuf, PACKET_LEN);
            // Serial.printf("-- encoded buf, enc_size = %d \n", packet_rle_size);
            // printPacket(packetBufEnc, packet_rle_size);
            // Serial.printf("-- decoded buf \n");
            // printPacket(packetBufDec, PACKET_LEN);
            delay(500);
          }
        }
        
        // sendPacket(packetBuf, PACKET_LEN);
        sendPacket(packetBuf, PACKET_LEN);
        sendBLE(packetBufEnc, PACKET_LEN);

        adc_scan_done = false;
        deliver_count_main++;
    }  // while
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

    int body_len = packet_buffer[4];
    Serial.printf(" adc data length : %d \n", body_len);

    if((MUX_LIST_LEN * NUM_MUX_OUT) == body_len) {
      for (int mux_id = (MUX_LIST_LEN - 1); 0 <= mux_id; mux_id--) {
          Serial.printf("[mux:%2d] ", mux_id);

          for (int i = 0; i < NUM_MUX_OUT; i++) {
              Serial.printf("%3d,", packet_buffer[HEADER_LEN + mux_id * NUM_MUX_OUT + i]);
          }
          Serial.println("~");
      }

      offset += (MUX_LIST_LEN * NUM_MUX_OUT);
    }
    else {
      for(int i = 0 ; i < body_len ; i++ ) {
        Serial.printf("%3d,", packet_buffer[HEADER_LEN + i]);
        if( ((i + 1) % 16) == 0 ) 
          Serial.println("~");
      }

      offset += body_len;
    }


    Serial.printf("[%d] %3d ", offset, packet_buffer[offset]);
    offset++;
    Serial.printf("[%d] %3d  \n", offset, packet_buffer[offset]);
    offset++;
}


void sendPacket(byte *packet_buffer, int packet_len) {
  //  send data to the PC
  Serial.write(packet_buffer, packet_len);
  //  log all data.
  // printPacket(packet_buffer, packet_len);

}

void sendBLE(byte *packet_buffer, int packet_len) {
  if(false == bleConnected) {
    return;
  }
  
  pCharacteristic->setValue(packet_buffer, (size_t)packet_len);
  pCharacteristic->notify();

}
