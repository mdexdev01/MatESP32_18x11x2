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

void setup() {
    Serial.setTxBufferSize(SERIAL_SIZE_TX);
    Serial.begin(BAUD_RATE0);

    Serial1.setRxBufferSize(SERIAL_SIZE_RX);
    Serial1.begin(BAUD_RATE1, SERIAL_8N1, 18, 17);

    memset(packetBuf, 0, PACKET_LEN);

    // Serial.printf("SETUP-GPIO \n");
    setup_gpioWork();

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
        10,           // 태스크 우선순위
        &Task_Core0,   // 태스크 핸들
        CORE_0);       // 실행될 코어

    afterSetup();
}

void afterSetup() {
    //------------------------------
    //  Check Dip switch
    adcScanMainPage();
    updateDipSwVal();
    // if(dip_decimal == 0) {
    //   changeTimerDurMS(40);
    // }
    // else {
    //   changeTimerDurMS(40);
    // }
}

int adc_scan_done = false;

int loop_count = 0;
void loop() {
    // Serial.printf("\nLOOP Count2 = %d \n", loop_count);
    if (adc_scan_done == false) {
        adcScanMainPage();
        reorder_sensor_shape();
    } else {
        delay(1);
    }

    loop_wdTimer();
    loop_gpioWork();
    updateDipSwVal();
    // loop_advGrib();

    loop_count++;
}

int GET_1of2(int num) {
    return (num % 2);
}

int adc_scan_count_main = 0;
void adcScanMainPage() {
    //  CHANGE MUX AND READ ADC 16EA
    for (int mux_id = 0; mux_id < MUX_LIST_LEN; mux_id++) {
        changeMux(mux_id);
        read_16ch_in_mux_fast(adc_value[mux_id]);  //  adc_value : 2d array
    }

    // Serial.println("measure done");

    adc_scan_done = true;
    adc_scan_count_main++;
}

int deliver_count_mine = 0;
int tx_timer_count_last = 0;
void pumpSerial(void *pParam) {
    Serial.printf("PUMP- ENTER\n");
    while (true) {
        //  Deilvery : Main job
        if (dip_decimal == 0) {  // Main board
            for (int i = 0; i < 3; i++) {
                while (true) {
                    int ret_val = MainBoard_Rx1();  // enable only for the Master

                    if (ret_val <= 0)
                        break;
                }
                delay(2);
            }
        } else if (dip_decimal == 1) {
            SubBoard_Rx1();
        } else if (dip_decimal == 2) {
            SubBoard_Rx1();
        }else if (dip_decimal == 3) {
            SubBoard_Rx1();
        }

        // Serial.printf("[%8dms] adc_done=%d, timer_done=%d, grant1=%d, grant2=%d \n",
        //               millis(), adc_scan_done, timer_flag, tx_grant_board1, tx_grant_board2);

        if (adc_scan_done == false) {
            delay(1);
            continue;
        }

        if (timer_flag == true) {
            //  Build and Send
            // buildPacket(packetBuf, adc_value, MUX_LIST_LEN, NUM_USED_OUT);

            // Serial.printf("TX timer loop count = %d \n", deliver_count_mine);

            switch (dip_decimal) {
                case 0:
                    // buildPacket_brandContents(packetBuf, adc_value, MUX_LIST_LEN, NUM_USED_OUT);
                    buildPacket_lgd(packetBuf, adc_value, MUX_LIST_LEN, NUM_USED_OUT);
                    sendPacket0(packetBuf, PACKET_LEN);
                    adc_scan_done = false;
                    timer_flag = false;
                    break;
                case 1:
                    if (tx_grant_board1 == true) {
                        // buildPacket_brandContents(packetBuf, adc_value, MUX_LIST_LEN, NUM_USED_OUT);
                        buildPacket_lgd(packetBuf, adc_value, MUX_LIST_LEN, NUM_USED_OUT);
                        sendPacket1(packetBuf, PACKET_LEN);
                        tx_grant_board1 = false;
                        tx_timer_count_last = deliver_count_mine;
                        adc_scan_done = false;
                        timer_flag = false;
                    }
                    break;
                case 2:
                    if (tx_grant_board2 == true) {
                        // buildPacket_brandContents(packetBuf, adc_value, MUX_LIST_LEN, NUM_USED_OUT);
                        buildPacket_lgd(packetBuf, adc_value, MUX_LIST_LEN, NUM_USED_OUT);
                        sendPacket1(packetBuf, PACKET_LEN);
                        tx_grant_board2 = false;
                        tx_timer_count_last = deliver_count_mine;
                        adc_scan_done = false;
                        timer_flag = false;
                    }
                    break;
                case 3:
                    if (tx_grant_board3 == true) {
                        // buildPacket_brandContents(packetBuf, adc_value, MUX_LIST_LEN, NUM_USED_OUT);
                        buildPacket_lgd(packetBuf, adc_value, MUX_LIST_LEN, NUM_USED_OUT);
                        sendPacket1(packetBuf, PACKET_LEN);
                        tx_grant_board3 = false;
                        tx_timer_count_last = deliver_count_mine;
                        adc_scan_done = false;
                        timer_flag = false;
                    }
                    break;
            }

            if (dip_decimal != 0) {
                if (3 < (deliver_count_mine - tx_timer_count_last)) {
                    Serial.printf("KEY hunger %d, ", (deliver_count_mine - tx_timer_count_last));
                    tx_grant_board1 = tx_grant_board2 = tx_grant_board3 = true;
                }
            }

            deliver_count_mine++;
        }

        tempDelay(1);

    }  // while
}

//  send data to the PC
void sendPacket0(byte *packet_buffer, int packet_len) {
    Serial.write(packet_buffer, packet_len);

    // Serial.printf("[%8dms] TX [%d->x], seq:%d>TX \n", millis(), packet_buffer[IDX_BOARD_ID], packet_buffer[IDX_RES_1]);
    //  log all data.
    // if(packet_buffer[IDX_BOARD_ID] == 0)
    //   printPacket(packet_buffer, packet_len);
}

//  send data to the PC
void sendPacket1(byte *packet_buffer, int packet_len) {
    digitalWrite(pin485U1DE, HIGH);  // LOW : RX, HIGH : TX
    // Serial1.printf("Hello \n");

    Serial1.write(packet_buffer, packet_len);
    // Serial.write(packet_buffer, packet_len);
    // delay(2);
    delayMicroseconds(200);         // 100: NG, 200:OK, 400: OK
    digitalWrite(pin485U1DE, LOW);  // LOW : RX, HIGH : TX

    Serial.printf("[%8dms] TX [%d->x], seq:%d>TX \n", millis(), packet_buffer[IDX_BOARD_ID], packet_buffer[IDX_RES_1]);

    //  log all data.
    // printPacket(packet_buffer, packet_len);
}
