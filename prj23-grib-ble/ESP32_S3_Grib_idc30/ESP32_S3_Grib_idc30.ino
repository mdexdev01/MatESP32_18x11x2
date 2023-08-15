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

    < Grib BLE Gateway connection >
    UART : 115,200bps
    Linux 로그인 ID는  grib, PW는 grib12!@ 입니다.
    로그인 후 shell이 나오면 다음과 같이 명령을 치면 gateway로그를 확인할 수 있습니다.
    journalctl -f -u grib-ble-sensor-gateway.service


  < TODO >
  1. Packet protocol document
//   2. Tx interval - 500ms
//   3. No data, No Tx
//   4. Switch 1, 2
  5. DFU

*/

#include "commPacket.h"
#include "lib_ble_grib.h"
#include "lib_gpio.h"
// #include "lib_wdtimer.h"

TaskHandle_t Task_Core0;
#define CORE_0 0
#define CORE_1 1

int blePacketSeq = 0;

void setup() {
    Serial.setTxBufferSize(SERIAL_SIZE_TX);
    Serial.begin(BAUD_RATE);

    Serial1.setRxBufferSize(SERIAL_SIZE_RX);
    Serial1.begin(BAUD_RATE, SERIAL_8N1, 18, 17);

    memset(packetBuf, 0, PACKET_LEN);

    Serial.printf("SETUP-GPIO \n");
    setup_gpioWork();

    Serial.printf("SETUP-HW PINS \n");
    setup_HWPins_Grib();
    selectMux(0);

    Serial.printf("SETUP-WD TIMER \n");
    setup_wdTimer();

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

    loop_wdTimer();
    loop_gpioWork();
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

        sendPacket(packetBuf, PACKET_LEN);
        sendBLEGrib(packetBuf, PACKET_LEN);

        adc_scan_done = false;
        deliver_count_main++;
        // tempDelay(500);
    }  // while
}

void sendPacket(byte *packet_buffer, int packet_len) {
    //  send data to the PC
    // Serial.write(packet_buffer, packet_len);

    //  log all data.
    // printPacket(packet_buffer, packet_len);
}

#define THRESHOLD_CUTOFF 400
bool isPassCutoff(byte *basket_buffer, int packet_len) {
    int body_len = basket_buffer[IDX_BODY_LEN];
    long sum_data = 0;

    for (int i = 0; i < body_len; i++) {
        sum_data += basket_buffer[i + HEADER_LEN];
        sum_data += basket_buffer[i + HEADER_LEN + PACKET_LEN];

        if (THRESHOLD_CUTOFF < sum_data) {
            return true;
        }
    }

    return false;
}

long saveTick = millis();
bool timeCert = true;
bool isBleSendTime() {
    long curTick = millis();
    long difTick = curTick - saveTick;

    if (500 < difTick) {
        // Serial.printf("diff = %d, curTick = %d, saveTick = %d \n", difTick, curTick, saveTick);
        saveTick += (500 * (difTick / 500));
        timeCert = true;
        return true;
    }

    return false;
}

#define BASKET_LEN (PACKET_LEN * 2)
byte packetBasket[BASKET_LEN];
#define ENC_BASKET_LEN (PACKET_LEN * 2 + 100)
byte EncBasket[ENC_BASKET_LEN];  // 100 : protect ble encoding buffer overflow...
byte blePacketGrib[BASKET_LEN + 100];

void sendBLEGrib(byte *packet_raw_buffer, int packet_len) {
    if (false == bleConnected) {
        return;
    }

    //  if time interval is short, then don't send to save network cost.
    if (false == isBleSendTime()) {
        return;
    } else {
        timeCert = false;
        // Serial.printf("curTick = %d \n", millis());
    }
    // Serial.printf("MY Data\t\t{\"id\": \"%02x%02x%02x\", \"seq\": %d, \"data\": \"", bleMacAddr[3], bleMacAddr[4], bleMacAddr[5], blePacketSeq);

    //  make basket buffer (basket buffer = top mat + bottom mat)
    int board_id = packet_raw_buffer[IDX_BOARD_ID];

    if (board_id == BOARD_ID_TOP) {
        memcpy(packetBasket, packet_raw_buffer, PACKET_LEN);
    } else if (board_id == BOARD_ID_BOTTOM) {
        memcpy(packetBasket + PACKET_LEN, packet_raw_buffer, PACKET_LEN);
    }

    //  if adc data is almost zero, then don't send to save network cost
    if (false == isPassCutoff(packetBasket, PACKET_LEN * 2)) {
        // Serial.printf("cutoff packet \n");
        return;
    }

    //  rle encoding
    int packet_rle_len = rle_encode(packetBasket, BASKET_LEN,
                                    EncBasket, ENC_BASKET_LEN);
    memcpy(blePacketGrib, &blePacketSeq, sizeof(int));
    memcpy(blePacketGrib + sizeof(int), EncBasket, packet_rle_len);

    //  send packet via BLE
    size_t ble_packet_size = (size_t)(packet_rle_len + sizeof(int));
    pCharacteristic->setValue(blePacketGrib, ble_packet_size);
    pCharacteristic->notify();

    {
        //  Raw Data        {"id": "fe0f99", "seq": 1280, "data": "0005000086ffff00~0000fe"}
        Serial.printf("Raw Data\t{\"id\": \"%02x%02x%02x\", \"seq\": %d, \"data\": \"", bleMacAddr[3], bleMacAddr[4], bleMacAddr[5], blePacketSeq);
        printHexa(blePacketGrib, ble_packet_size);
        Serial.println("\"}");

        //  Indicate        {"id": "fe0f99", "seq": 1280, "data": "ffff0000b0000000~0000fe"}
        Serial.printf("Indicate\t{\"id\": \"%02x%02x%02x\", \"seq\": %d, \"data\": \"", bleMacAddr[3], bleMacAddr[4], bleMacAddr[5], blePacketSeq);
        printHexa(packetBasket, PACKET_LEN * 2);
        Serial.println("\"}");
    }

    blePacketSeq++;
}

void sendBLEGrib_OnePad(byte *packet_raw_buffer, int packet_len) {
    if (false == bleConnected) {
        return;
    }
    int packet_rle_len = rle_encode(packet_raw_buffer, packet_len,
                                    enc_buffer, ENC_BUF_SIZE);
    memcpy(blePacketGrib, &blePacketSeq, sizeof(int));
    memcpy(blePacketGrib + sizeof(int), enc_buffer, packet_rle_len);

    int packet_size = (size_t)packet_rle_len + sizeof(int);
    pCharacteristic->setValue(blePacketGrib, packet_size);
    pCharacteristic->notify();

    /*
      Serial.println("\nprint Enc");
      printHexa(blePacketGrib, packet_size);
      Serial.println("\nprint Ori");
      printHexa(packet_raw_buffer, packet_len);
    */

    blePacketSeq++;
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

    int body_len = packet_buffer[IDX_BODY_LEN];
    Serial.printf(" adc data length : %d \n", body_len);

    if ((MUX_LIST_LEN * NUM_MUX_OUT) == body_len) {
        for (int mux_id = (MUX_LIST_LEN - 1); 0 <= mux_id; mux_id--) {
            Serial.printf("[mux:%2d] ", mux_id);

            for (int i = 0; i < NUM_MUX_OUT; i++) {
                Serial.printf("%3d,", packet_buffer[HEADER_LEN + mux_id * NUM_MUX_OUT + i]);
            }
            Serial.println("~");
        }

        offset += (MUX_LIST_LEN * NUM_MUX_OUT);
    } else {
        for (int i = 0; i < body_len; i++) {
            Serial.printf("%3d,", packet_buffer[HEADER_LEN + i]);
            if (((i + 1) % 16) == 0)
                Serial.println("~");
        }

        offset += body_len;
    }

    Serial.printf("[%d] %3d ", offset, packet_buffer[offset]);
    offset++;
    Serial.printf("[%d] %3d  \n", offset, packet_buffer[offset]);
    offset++;
}
