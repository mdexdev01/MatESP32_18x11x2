/*
  CUSTOMER : LUXTEP
  CHIPSET  : ESP32 S3 N16R8
  CAUTION !!!! - BOARD LIBRARY VERSION : 2.0.17  (NOT 3.0)

  Luxtep_CES_FW_01_d02_release.zip

  < TODO >
  2024-12-18
    [O]ADC NOISE,
    [O]ADC speed
    [O]RX BUFFER OVERFLOW/CRASH,
    [O]Crop LED
    NAND, PSRAM working
    OSD RGB full color buffer
    message handler
    bmp display
    OTA

*/
#include <NeoPixelBus.h>
// #include <Adafruit_NeoPixel.h>
// #ifdef __AVR__
// #include <avr/power.h>  // Required for 16 MHz Adafruit Trinket
// #endif

#include "commPacket.h"
#include "configPins-mdll-24-6822.h"
#include "libBuzzer.h"
#include "libLED_Object.h"
#include "libProtocol.h"
#include "lib_ble_ota.h"
#include "lib_gpio.h"
#include "lib_ledworks_28x35.h"

bool is_ManualMode = false;  // false : Full frame, true : just one point measurement.

TaskHandle_t Task_Core0;
#define CORE_0 0
#define CORE_1 1

void setup() {
    // #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
    // clock_prescale_set(clock_div_1);
    // #endif
    // END of Trinket-specific code.
    Serial.setTxBufferSize(SERIAL_SIZE_TX);
    Serial.setRxBufferSize(SERIAL_SIZE_RX);
    Serial.begin(BAUD_RATE0, SERIAL_8N1);

    Serial1.setRxBufferSize(SERIAL_SIZE_TX);
    Serial1.setRxBufferSize(SERIAL_SIZE_RX);
    Serial1.begin(BAUD_RATE1, SERIAL_8N1, 18, 17);  // 18 : RXD, 17 : TXD

    setup_Comm();

    memset(packetBuf, 0, PACKET_LEN);
    memset(packetBufOSD, 0, PACKET_HEAD_LEN + SUB_HEAD_LEN + OSD_BUFFER_SIZE);
    osd_buf = packetBufOSD + PACKET_HEAD_LEN + SUB_HEAD_LEN;

    //-------------------------------------------
    Serial.printf("SETUP-HW PINS \n");

    // GPIO 47 비활성화 - 예외 처리
    {
        gpio_config_t io_conf_47 = {};
        io_conf_47.pin_bit_mask = (1ULL << GPIO_NUM_47);
        io_conf_47.mode = GPIO_MODE_INPUT;  // Hi-Z 상태로 설정
        io_conf_47.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf_47.pull_up_en = GPIO_PULLUP_DISABLE;
        gpio_config(&io_conf_47);
    }

    //  mux
    setup_HWPins_34x28();
    selectPwrMux(0);  // 0 : temporary. one and only mux
    selectSenMux(0);  // 0 : temporary. one and only mux

    //  gpio, dip switch
    setup_gpioWork();
    read_dipsw();
    dip_decimal = binDip4;

    //  --------------------------------------------
    //  WELCOME
    //  Buzzer
    if (dip_decimal == 0) {
        setup_song();
        play_song();
    }

    //  LED strip - WS2812C-2020-V1
    // 모든 스트립 초기화 - LCD I/f library
    for (int i = 0; i < NUM_strip; i++) {
        strip[i].Begin();
        strip[i].ClearTo(RgbColor(0, 0, 0));
        strip[i].Show();
    }

    /*
        //  Adafruit library
        strip[0].setPin(DI0);
        strip[1].setPin(DI1);
        strip[2].setPin(DI2);
        strip[3].setPin(DI3);

        byte bright_level = 20;
        for (int i = 0; i < 4; i++) {  // not 3, but 4
            strip[i].updateType(NEO_GRB + NEO_KHZ800);

            if (i == 3)
                strip[3].updateLength(LED_COUNT_3);
            else
                strip[i].updateLength(LED_COUNT);

            strip[i].begin();                              // INITIALIZE NeoPixel strip object (REQUIRED)
            strip[i].setBrightness(bright_level);          // Set BRIGHTNESS to about 1/5 (max = 255)
                                                           // strip[i].show();                       // Turn OFF all pixels ASAP
            strip[i].fill(strip[i].Color(255, 255, 255));  // 흰색으로 설정
        }
    */

    // intro_led();

    //------------------------------
    //  TASKS - 00
    Serial.printf("TASK CREATES \n");
    xTaskCreatePinnedToCore(
        pumpSerial,    // 태스크 함수
        "pumpSerial",  // 테스크 이름
        (1024 * 14),   // 스택 크기(워드단위)
        NULL,          // 태스크 파라미터
        10,            // 태스크 우선순위
        &Task_Core0,   // 태스크 핸들
        CORE_0);       // 실행될 코어
}

#define rx_buf_len 32
char read_data[rx_buf_len];
int ext_ch = 0, sen_ch = 0;

int serial_rx() {
    // while(Serial.available()) {
    //   char read_data = Serial.read();
    //   Serial.printf("[RX] %s\n", read_data);
    // }

    while (Serial.available()) {
        memset(read_data, 0, rx_buf_len);
        int read_len = Serial.readBytes(read_data, rx_buf_len);
        //  remove spaces
        {
            Serial.printf("RX:%s", read_data);
            char *output = read_data;
            int i = 0, j = 0;

            while (read_data[i] != '\0') {
                if (read_data[i] != ' ') {
                    output[j] = read_data[i];
                } else {
                    j--;
                }

                i++;
                j++;
            }
            read_data[j] = '\0';
            // Serial.printf("RX:%s", read_data);
        }

        sscanf(read_data, "%d,%d", &ext_ch, &sen_ch);
        Serial.printf("ext_ch=%d, sen_ch=%d\n", ext_ch, sen_ch);
    }

    return 0;
}

void intro_led() {
    // Fill along the length of the strip in various colors...
    // colorWipe(strip[i].Color(255,   0,   0), 50); // Red
    // colorWipe(strip[i].Color(  0, 255,   0), 50); // Green
    // colorWipe(strip[i].Color(  0,   0, 255), 50); // Blue

    // // Do a theater marquee effect in various colors...
    // theaterChase(strip[i].Color(127, 127, 127), 50); // White, half brightness
    // theaterChase(strip[i].Color(127,   0,   0), 50); // Red, half brightness
    // theaterChase(strip[i].Color(  0,   0, 127), 50); // Blue, half brightness

    // rainbow(10);  // Flowing rainbow cycle along the whole strip

    theaterChaseRainbow(100);  // Rainbow-enhanced theaterChase variant
}

long loop_count = 0;
int adc_scan_done = false;

void draw_ledObjects() {
    blurObjectOutline();  // blur outline
    clearPixels();
    // loop_fsrled(SIZE_X, SIZE_Y);  // draw COP
    // loop_balanceWorks(SIZE_X, SIZE_Y);

    drawIndicator();
    drawPixels();
}

void loop() {
    // Serial.printf("[%08d] adc go \n", millis());
    if (is_ManualMode == false)
        adcScanMainPage();
    // Serial.printf("[%08d] adc end \n", millis());

    reorderADC2LED();  // change X, Y coord

    read_dipsw();
    dip_decimal = binDip4;
    // Serial.printf("\nboard id = %d\n", dip_decimal);

    loop_gpioWork();  // check buttons
    //  check ota
    // bool is_ota = checkOTAProc();

    if (dip_decimal == 1110) {
        MAIN_RX0_TX1();
        delayMicroseconds(10);
        MAIN_RX0_TX1();
        delayMicroseconds(10);
    }

    // Serial.printf("[%08d] draw go \n", millis());
    draw_ledObjects();
    // Serial.printf("[%08d] draw end \n", millis());
    vTaskDelay(8);
    vTaskDelay(26);

    if (dip_decimal == 1110) {
        MAIN_RX0_TX1();
        delayMicroseconds(10);
        MAIN_RX0_TX1();
        delayMicroseconds(10);
    }

    // buildPacket_Luxtep(packetBuf, ForceData, NUM_LUXTEP_WIDTH, NUM_LUXTEP_HEIGHT);
    // sendPacket0(packetBuf, PACKET_LEN);
    // printArray_34x28();

    taskYIELD();
    loop_count++;
}
int deliver_count_mine = 0;
int tx_timer_count_last = 0;

void pumpSerial(void *pParam) {
    Serial.printf("PUMP- ENTER\n");

    while (true) {
        int ret_val = 0;
        //  Deilvery : Main job
        if (dip_decimal == 0) {  // Main board
            MAIN_TX1();
            for (int i = 0; i < 3; i++) {
                while (true) {
                    ret_val = MAIN_RX1();  // temp----------
                    delayMicroseconds(10);

                    if (ret_val <= 0) {
                        break;
                    }
                }
            }
            MAIN_TX1();
            // if (send_tx1 == true) {
            //     MAIN_TX1();
            // }
        } else if ((1 <= dip_decimal) && (dip_decimal <= 8)) {
            for (int i = 0; i < 4; i++) {
                ret_val = SUB_RX1(i);
                delayMicroseconds(10);
            }
        }

        if (dip_decimal == 0) {
            MAIN_TX1();
            for (int i = 0; i < 4; i++) {
                MAIN_RX0_TX1();
                delayMicroseconds(10);
            }
            MAIN_TX1();
        }

        // Serial.printf("[%8dms] adc_done=%d, timer_done=%d, grant1=%d, grant2=%d \n",
        //               millis(), adc_scan_done, timer_flag, tx_grant_board1, tx_grant_board2);
        taskYIELD();

        if (adc_scan_done == false) {
            delayMicroseconds(10);
            continue;
        }

        //  DRAW LED
        // draw_ledObjects();

        boolean timer_flag = true;  // temp----------

        // delay(10);
        tx_grant_board1 = true;

        if (timer_flag == true) {
            //  Build and Send
            // buildPacket(packetBuf, adc_value, MUX_LIST_LEN, NUM_USED_OUT);

            // Serial.printf("TX timer loop count = %d (dip=%d) \n", deliver_count_mine, dip_decimal);

            switch (dip_decimal) {
                case 0:
                    buildPacket_Luxtep(packetBuf, ForceData, NUM_LUXTEP_WIDTH, NUM_LUXTEP_HEIGHT);
                    sendPacket0(packetBuf, PACKET_LEN);
                    adc_scan_done = false;
                    timer_flag = false;
                    break;
                case 1:
                    if ((tx_grant_board1 == true) || (0 < consumeTimerToken())) {
                        buildPacket_Luxtep_Sub(packetBuf, ForceData, SIZE_X, NUM_LUXTEP_HEIGHT);
                        sendPacket1(packetBuf, SUB_PACKET_LEN);
                        tx_grant_board1 = false;
                        tx_timer_count_last = deliver_count_mine;
                        adc_scan_done = false;
                        timer_flag = false;
                    }
                    break;
                case 2:
                    if (tx_grant_board2 == true) {
                        buildPacket_Luxtep(packetBuf, ForceData, NUM_LUXTEP_WIDTH, NUM_LUXTEP_HEIGHT);
                        sendPacket1(packetBuf, PACKET_LEN);
                        tx_grant_board2 = false;
                        tx_timer_count_last = deliver_count_mine;
                        adc_scan_done = false;
                        timer_flag = false;
                    }
                    break;
                case 3:
                    if (tx_grant_board3 == true) {
                        buildPacket_Luxtep(packetBuf, ForceData, NUM_LUXTEP_WIDTH, NUM_LUXTEP_HEIGHT);
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
                    // Serial.printf("KEY hunger %d, ", (deliver_count_mine - tx_timer_count_last));
                    tx_grant_board1 = tx_grant_board2 = tx_grant_board3 = true;
                }
            }
        }

        deliver_count_mine++;
        vTaskDelay(1);
    }  // while
}

//	이거 PWR를 고정하고 센서를 움직이는데, 반대로 센서를 고정하고 PWR를 바꾸는 방식으로 수정해야 함.
int adc_scan_count_main = 0;
void adcScanMainPage() {
    //  for ext mux id loop
    for (int col_pwr = 0; col_pwr < NUM_PWR; col_pwr++) {
        selectPwrCh(col_pwr);

        readSen1Col(col_pwr);  // temp

    }  // end - for ext mux id loop

    adc_scan_done = true;
}

void adcScanMainPage_Slow() {
    //  for ext mux id loop
    for (int col_pwr = 0; col_pwr < NUM_PWR; col_pwr++) {
        selectPwrCh(col_pwr);

        //  for sen mux id loop
        for (int row_sen = 0; row_sen < NUM_SEN; row_sen++) {
            readSenCh(row_sen);
            VOut_Value[col_pwr][row_sen] = _vout_raw;
            VRef_Value[col_pwr][row_sen] = _vref_raw;
            ForceData[col_pwr][row_sen] = calcF(_vout_raw, _vref_raw);

        }  //  end - for sen mux id loop
    }  // end - for ext mux id loop

    adc_scan_done = true;
}

void measure1ch(int ext_ch, int sen_ch) {
    selectPwrCh(ext_ch);
    read1chInSenMux(sen_ch);
    // read8chInSenMux(ext_ch);

    Serial.printf("[E%d,S%d]O:%4d, R:%4d \n", ext_ch, sen_ch, _vout_raw, _vref_raw);
}

void printArray_34x28() {
    Serial.printf("\n[%d]---------34 x 28---------------\n", loop_count);
    for (int row_sen = NUM_SEN - 1; -1 < row_sen; row_sen--) {
        for (int col_pwr = 0; col_pwr < NUM_PWR; col_pwr++) {
            Serial.printf(" %3d ", ForceData[col_pwr][row_sen]);
        }
        Serial.printf("\n");
    }

    for (int row_sen = NUM_SEN - 1; -1 < row_sen; row_sen--) {
        for (int col_pwr = 0; col_pwr < NUM_PWR; col_pwr++) {
            int led_index = row_sen * NUM_PWR + col_pwr;
            Serial.printf("[%3d]", inpol_buf[led_index]);
        }
        Serial.printf("\n");
    }

    // for (int row_sen = NUM_SEN - 1; -1 < row_sen; row_sen--) {
    //     for (int col_pwr = 0; col_pwr < NUM_PWR; col_pwr++) {
    // int r_value = calcR(VOut_Value[col_pwr][row_sen], VRef_Value[col_pwr][row_sen]);
    // Serial.printf("[%2d,%2d] %4d", col_pwr, row_sen, r_value);

    // int f_value = calcF(VOut_Value[col_pwr][row_sen], VRef_Value[col_pwr][row_sen]);
    // Serial.printf("[%2d,%2d] %4d", col_pwr, row_sen, f_value);

    // Serial.printf("[%d, %d] %4d /%4d", col_pwr, row_sen, VOut_Value[col_pwr][row_sen], VRef_Value[col_pwr][row_sen]);
    // }
    // Serial.printf("\n");
    // }
}
