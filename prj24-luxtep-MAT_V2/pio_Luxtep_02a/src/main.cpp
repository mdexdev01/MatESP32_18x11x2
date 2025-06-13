/*
  CUSTOMER : LUXTEP
  CHIPSET  : ESP32 S3 N16R8
  CAUTION !!!! - BOARD LIBRARY VERSION : esp32 2.0.17  (NOT 3.0)

  Luxtep_CES_FW_01_d02_release.zip

  < TODO >
  2024-12-18
    [O] ADC NOISE,
    [O] ADC speed 3M
    [O] RX BUFFER OVERFLOW/CRASH,
    [O] Crop LED
    [O] UART0, UART1 isr event handeler
    [O] message handler, 25/01/25
  2025-02-20
    [O]Double Buffering : <SUB> ADC <--> LED, TX1
    [O]Double Buffering : <MAIN> ADC <--> LED, TX0
    [O]Double Buffering : <MAIN> UART1-x-UART0

    UART0, 1의 TX를 NON-BLOCKING으로 변경
    pump, adc, drawLED에 대해 시간 최적화시에 0~4열 노이즈 발생
    permit 메시지 사용
    1SET, 1BD 패킷 구조 변경


    Task memory optimize by profiling
    LED garbage while rx osd buffer.

    NAND, PSRAM working
    bmp display

    WIFI-BLE AP config
    WIFI Connectivity
    OSD RGB full color buffer

    OTA
    Library and API

*/
#include <Arduino.h>
#include <esp_log.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <Update.h>

#include "lib_wifiConfig.h"
#include "lib_ota.h"
#include "commPacket.h"
#include "configPins-mdll-24-6822.h"
#include "driver/ledc.h"
#include "libLED_Object.h"
#include "libPrintRaw.h"
#include "libProtocol.h"
#include "packetBuffer.h"
// #include "lib_ble_ota.h"
#include "lib_buzzer.h"
#include "lib_gpio.h"
#include "lib_ledworks_28x35.h"

#define FIRMWARE_VERSION "0.2.0-WIFI-OTA-20250609"

bool is_ManualMode = false;  // false : Full frame, true : just one point measurement.

TaskHandle_t Task_Core0;
TaskHandle_t hTask_UART0;
TaskHandle_t hTask_UART1;

SemaphoreHandle_t semaForceBuf_rd;  // 세마포어 핸들 생성

#define CORE_0 0
#define CORE_1 1

void loopADCRead(void *pvParameters);
void loopDrawLED(void *pvParameters);
void readDipSwitch();

void setup() {
    // UART0 설정
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
    uart_driver_install(UART_NUM_0, SERIAL_SIZE_RX * 2, SERIAL_SIZE_TX, 20, &UART0_EventQueue, 0);  // 20 = queue_size, 0 = intr_alloc_flags

    //  UART 버퍼 초기화
    uart_wait_tx_done(UART_NUM_0, pdMS_TO_TICKS(100));  // TX 버퍼 초기화
    uart_flush_input(UART_NUM_0);                       // RX 버퍼 초기화
    uart_flush(UART_NUM_0);

    // RX1 pin pull up mode
    gpio_set_pull_mode((gpio_num_t)RX_PIN, GPIO_PULLUP_ONLY);  // RX pin pull up mode

    // UART1 설정
    uart_config_t uart_config_1 = {
        .baud_rate = BAUD_RATE1,  // 3Mbps 설정
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 96, // 122
        .source_clk = UART_SCLK_APB};

    uart_param_config(UART_NUM_1, &uart_config_1);
    uart_set_pin(UART_NUM_1, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_1, SERIAL_SIZE_RX * 8, SERIAL_SIZE_TX, 20 * 10, &UART1_EventQueue, 0);  // 20 = queue_size, 0 = intr_alloc_flags
    
    uart_set_rx_full_threshold(UART_NUM_1, 64);

    // pinMode(RX_PIN, INPUT_PULLUP);  // RX pin pull up mode

    // RX1 이벤트 비활성화
    disable_uart_rx_events(UART_NUM_1);

    //  UART 버퍼 초기화
    uart_wait_tx_done(UART_NUM_1, pdMS_TO_TICKS(100));  // TX 버퍼 초기화
    uart_flush_input(UART_NUM_1);                       // RX 버퍼 초기화
    uart_flush(UART_NUM_1);
    /*
        // 첫 번째 수신 데이터 무조건 버리기 (패킷 꼬임 방지)
        uint8_t dummy_buf[1024];
        uart_read_bytes(UART_NUM_1, dummy_buf, 1024, pdMS_TO_TICKS(50));
    */
    //-------------------------------------------
    uart0_printf("FIRMWARE VERSION - %s \n", FIRMWARE_VERSION);
    uart0_printf("SETUP-HW PINS \n");

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
    uart0_printf("set up buffer... \n");
    setup_MeasureBuffers();
    setup_LEDBuffers();

    uart0_printf("gpio 34 x 28 \n");
    setup_HWPins_34x28();
    uart0_printf("mux initialize... \n");
    ext_mux_id_prev = NUM_PWR_MUX - 1;
    selectPwrMux(0);  // 0 : temporary. one and only mux
    en_sen_mux_id = NUM_SEN_MUX - 1;
    selectSenMux(0);  // 0 : temporary. one and only mux

    //  gpio, dip switch
    uart0_printf("gpio else \n");
    setup_gpioWork();
    read_dipsw(false);
    MY_BOARD_ID = binDip4;

    //  ----------END OF HARDWARE SETTING ----------
 
    //  adc, osd, packet buffer
    setup_PacketBuffer(MY_BOARD_ID);

    //  Set RS485 RX mode
    setup_RS485();

    //------------------------------
    //  SEMAPHORES

    semaForceBuf_rd = xSemaphoreCreateBinary();  // 바이너리 세마포어 생성 (초기 상태 : 점유)
    if (semaForceBuf_rd == NULL) {
        Serial.println("Error: Unable to create semaphore!, semaForceBuf_rd");
        while (1);
    }

    semaSendPermit = xSemaphoreCreateBinary();  // 바이너리 세마포어 생성 (초기 상태 : 점유)
    if (semaSendPermit == NULL) {
        Serial.println("Error: Unable to create semaphore!, semaSendPermit");
        while (1);
    }
    xSemaphoreGive(semaSendPermit);

    semaTX1 = xSemaphoreCreateBinary();  // 바이너리 세마포어 생성 (초기 상태 : 점유)   
    if (semaTX1 == NULL) {
        Serial.println("Error: Unable to create semaphore!, semaTX1");
        while (1);
    }
    xSemaphoreGive(semaTX1);

    
    uart0_printf("PLAY SONG \n");
    //  --------------------------------------------
    //  WELCOME
    //  Buzzer
    if (MY_BOARD_ID == 0) {
        setup_song();
        play_song();
    }

    uart0_printf("LED WELCOME \n");
    //  LED strip - WS2812C-2020-V1
    /*
        {
    #define LEDC_TIMER LEDC_TIMER_0
    #define LEDC_MODE LEDC_LOW_SPEED_MODE
    #define LEDC_DUTY_RES LEDC_TIMER_10_BIT  // 10-bit 해상도 (0~1023)
    #define LEDC_FREQUENCY 5000              // 5kHz PWM 주파수

            // 사용할 GPIO 핀 및 LEDC 채널
            const int led_pins[] = {8, 21, 14, 3};
            const ledc_channel_t led_channels[] = {LEDC_CHANNEL_0, LEDC_CHANNEL_1, LEDC_CHANNEL_2, LEDC_CHANNEL_3};

            {
                // LEDC 타이머 설정
                ledc_timer_config_t ledc_timer = {
                    .speed_mode = LEDC_MODE,
                    .duty_resolution = LEDC_DUTY_RES,
                    .timer_num = LEDC_TIMER,
                    .freq_hz = LEDC_FREQUENCY,
                    .clk_cfg = LEDC_AUTO_CLK,
                };
                ledc_timer_config(&ledc_timer);

                // 여러 개의 GPIO에 PWM 설정
                for (int i = 0; i < 4; i++) {
                    ledc_channel_config_t ledc_channel = {
                        .gpio_num = led_pins[i],  // GPIO 설정
                        .speed_mode = LEDC_MODE,
                        .channel = led_channels[i],
                        .intr_type = LEDC_INTR_DISABLE,  // ⚠ 순서 변경
                        .timer_sel = LEDC_TIMER,
                        .duty = 0,  // 초기 Duty Cycle = 0
                        .hpoint = 0};
                    ledc_channel_config(&ledc_channel);
                }
            }
        }
    */
    // 모든 스트립 초기화 - LCD I/f library
    for (int i = 0; i < NUM_strip; i++) {
        strip[i].Begin();
        strip[i].ClearTo(RgbColor(0, 0, 0));
        // strip[i].Show();
        delay(2);
    }

    //  Setup WIFI
    if(dip_state[4] == LOW) // HIGH : SERIAL MODE
        setup_tcpStar();

    //------------------------------
    //  TASKS - 00

    uart0_printf("TASK CREATES \n");
    if (pdPASS != xTaskCreatePinnedToCore(
                      pumpSerial,    // 태스크 함수
                      "pumpSerial",  // 테스크 이름
                      (1024 * 14),   // 스택 크기(워드단위)
                      NULL,          // 태스크 파라미터
                      10 + 0,        // 태스크 우선순위
                      &Task_Core0,   // 태스크 핸들
                      CORE_0)) {     // 실행될 코어 0
        uart0_printf("ERROR - TASK CREATES pumpSerial \n");
    }
    // LOOP 태스크 생성
    if (pdPASS != xTaskCreatePinnedToCore(
                      loopADCRead,    // 태스크 함수
                      "loopADCRead",  // 태스크 이름
                      1024 * 4 * 10,  // 스택 크기
                      NULL,           // 태스크 파라미터
                      10 + 0,             // 태스크 우선순위
                      NULL,           // 태스크 핸들
                      CORE_1)) {      // 실행될 코어 1
        uart0_printf("ERROR - TASK CREATES loopADCRead \n");
    }
/*
    // LOOP 태스크 생성
    if (pdPASS != xTaskCreatePinnedToCore(
                      loopDrawLED,    // 태스크 함수
                      "loopDrawLED",  // 태스크 이름
                      1024 * 4,       // 스택 크기
                      NULL,           // 태스크 파라미터
                      10,             // 태스크 우선순위
                      NULL,           // 태스크 핸들
                      CORE_1)) {      // 실행될 코어 1
        uart0_printf("ERROR - TASK CREATES loopDrawLED \n");
    }
*/
    // UART0 이벤트 처리 태스크 생성
    if (pdPASS != xTaskCreatePinnedToCore(
                      uart0_event_task,
                      "uart0_event_task",
                      1024 * 8,
                      NULL,
                      10 + 19,
                      &hTask_UART0,
                      CORE_0)) {  // 실행될 코어 0
        uart0_printf("ERROR - TASK CREATES uart0_event_task \n");
    }

    // UART1 이벤트 처리 태스크 생성
    if (pdPASS != xTaskCreatePinnedToCore(
                      uart1_event_task,
                      "uart1_event_task",
                      1024 * 8,
                      NULL,
                      10 + 20,  // priority high
                      &hTask_UART1,
                      CORE_0)) {  // 실행될 코어 0
        uart0_printf("ERROR - TASK CREATES uart1_event_task \n");
    }

    if (pdPASS != xTaskCreatePinnedToCore(
                    receiverTask, 
                    "RXTask", 
                    4096, 
                    NULL, 
                    1, 
                    &receiverTaskHandle, 
                    CORE_1)) {
        uart0_printf("ERROR - TASK CREATES receiverTask \n");
    }

    setup_ota();
}

void loop() {
    vTaskDelay(1); // loop() task 초입 딜레이

    if(isOTACommand == true) {
        loop_ota();

        isOTACommand = false;
        //  OTA
        uart0_printf("OTA command \n");
    }

    if(dip_state[4] == HIGH) // HIGH : SERIAL MODE
        return;

    if(onWifiSetting == true)
        return;

    loop_tcpStar();

    return;
}

//---------------------------------------------
//  ADC SCAN TASK
//---------------------------------------------
long loop_count = 0;

void loopADCRead(void *pvParameters) {
    while (true) {
        vTaskDelay(1);  // loopADCRead() task 초입 딜레이

        if(isOTACommand == true)
            continue;

        //      CHECK if UART1 is currently used.
        if ((isBoard0_duringPermitCycle == true) && (MY_BOARD_ID == 0)) {
            #define NO_PERMIT_ACK_WAIT 120
            if (millis() - tickPermitLast < NO_PERMIT_ACK_WAIT) { // 데이터 에러로 인해 PERMIT의 ACK DATA를 받지 못함. 대개는 0번 보드 수신중에 BREAK 발생.
                continue;
            }
            uart0_printf("No permit ack wait %d ms \n", NO_PERMIT_ACK_WAIT);
            isBoard0_duringPermitCycle = false;
        }

        if(isSensorDataFilled == true) 
            continue;

        //-   SCAN ADC
        {
            uart0_printf("A");
            int cur_ms = millis();
            // uart0_printf("[%8d] adc start \n", millis());

            adcScan_DoubleBuf();  //  IT CONSUMES 48ms at least. And 66ms, with taskYIELD() in the function.
            taskYIELD();

            fillADC2LED();        // fill ledSrcBuf from forceBuffer_rd
            // uart0_printf("[%8d] adc ends, dur = %d ms, ptr:%d \n", millis(), millis() - cur_ms, forceBuffer_wr_last);

            //-  CHECK HW BUTTONS
            loop_gpioWork();  // check buttons
            readDipSwitch();  // read dip switch
            // checkOTAProc();   // check OTA
    
            uart0_printf("a");
        }

        //-  CHECK OTA
        if((TactButtons[0]->isClickedVeryLong == true) && (TactButtons[2]->isClickedVeryLong == true)) {
            isOTACommand = true;
            // uart0_printf("OTA Button clicked !!! \n");
            delay(2000);
        }
        taskYIELD();

        // delay(20);  // 20ms 

        //-  DRAW LED
        if(isNoLEDMode == false){
            draw_ledObjects();  // RMT transfer. IT returns in 2ms BUT internally CONSUMES 28ms at least. (in case 10ms??)
            // xSemaphoreGive(semaForceBuf_rd);  // ✅ Task1이 직접 해제
        }

        isSensorDataFilled = true; //~~~ Trigger to send data to use UART0 and UART1. 이곳에 두면 115ms 추후 별개 과제로 추진. 

        taskYIELD();
        // To prevent TX1 inter board deadlock.
        considerToPermit(190); 

        loop_count++;

        // uart0_printf("Task loopADCRead stack size: %d\n", uxTaskGetStackHighWaterMark(NULL));
    }

    return;
}

void readDipSwitch() {
    if (loop_count % 100 == 0) { // read dip switch
        read_dipsw(true);

        MY_BOARD_ID = binDip4;

        if(dip_state[4] == true){ // HIGH
            // SERIAL MODE
        }
        else{ // LOW
            // WIFI MODE
        }
        
        if(dip_state[5] == true){ // HIGH
            BoardID_MAX = 7;
        }
        else{ // LOW
            BoardID_MAX = 1;
            // BoardID_MAX = 7; // for test
        }
    }
}

//---------------------------------------------
//  Draw LED Task
//---------------------------------------------
int loop_led_count = 0;
void loopDrawLED(void *pvParameters) {
    while (true) {
        //      DISABLE THIS TASK
        vTaskDelay(1);  // loopDrawLED() task 초입 딜레이이
        // continue;

        if (xSemaphoreTake(semaForceBuf_rd, portMAX_DELAY) == pdTRUE) {
            //  DRAW LED ==> make this block to task on core 1. (Core1 : draw_ledObjects and adcScanMainPage)
            {
                int cur_ms = millis();
                draw_ledObjects();  // dma transfer. IT returns in 2ms BUT internally CONSUMES 28ms at least. (in case 10ms??)
            }

            loop_led_count++;
        }
    }

    return;
}

void printArray_34x28() {
    uart0_printf("\n[%d]---------34 x 28---------------\n", loop_count);
    for (int row_sen = NUM_SEN - 1; -1 < row_sen; row_sen--) {
        for (int col_pwr = 0; col_pwr < NUM_PWR; col_pwr++) {
            uart0_printf(" %3d ", calcedVolt[col_pwr][row_sen]);
        }
        uart0_printf("\n");
    }

    for (int row_sen = NUM_SEN - 1; -1 < row_sen; row_sen--) {
        for (int col_pwr = 0; col_pwr < NUM_PWR; col_pwr++) {
            int led_index = row_sen * NUM_PWR + col_pwr;
            uart0_printf("[%3d]", ledBlurBuf[led_index]);  // rd
        }
        uart0_printf("\n");
    }

    // for (int row_sen = NUM_SEN - 1; -1 < row_sen; row_sen--) {
    //     for (int col_pwr = 0; col_pwr < NUM_PWR; col_pwr++) {
    // int r_value = calcR(VOut_Value[col_pwr][row_sen], VRef_Value[col_pwr][row_sen]);
    // uart0_printf("[%2d,%2d] %4d", col_pwr, row_sen, r_value);

    // int f_value = calcF(VOut_Value[col_pwr][row_sen], VRef_Value[col_pwr][row_sen]);
    // uart0_printf("[%2d,%2d] %4d", col_pwr, row_sen, f_value);

    // uart0_printf("[%d, %d] %4d /%4d", col_pwr, row_sen, VOut_Value[col_pwr][row_sen], VRef_Value[col_pwr][row_sen]);
    // }
    // uart0_printf("\n");
    // }
}
