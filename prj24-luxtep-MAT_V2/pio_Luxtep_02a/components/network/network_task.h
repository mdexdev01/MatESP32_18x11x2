// network_task.h
// Version: Ver.2.4 (Dynamic Board ID)
// Author: mdex.co.kr & Refactored by AI
// Date: 2025-07-22
// 역할: 모든 모듈을 조율하고 프로그램의 실질적인 흐름을 관리합니다.

#ifndef _NETWORK_TASK_H_
#define _NETWORK_TASK_H_

// --- 표준 및 외부 라이브러리 ---
#include <WiFi.h>
#include <PubSubClient.h>
#include "time.h"

// --- 커스텀 라이브러리 ---
#include "network_info.h"
#include "libPrintRaw.h"
#include "lib_nvs.h"
#include "lib_wifi.h"
#include "lib_softapWebServer.h"
#include "lib_mqttHive.h"
#include "lib_tcpClient.h"
#include "lib_otaFirebase.h"

// ====================================================================
// --- 전역 변수 정의 (조율자 소유) ---
// ====================================================================
const char *ntpServer = "pool.ntp.org";
String mqttClientId;
String myFormattedMacAddress;
String currentAppID;
String receivedTcpServerIpAddress;
String storedTcpServerIp;
unsigned long bootTimestamp = 0;
String serialCommand = "";
bool isOTACommand = false;
bool otaUpdateRequested = false;

// ====================================================================
// --- Extern 객체 및 변수 선언 ---
// ====================================================================
extern PubSubClient client;
extern bool shouldEnterSoftAP;
extern WiFiClient tcpClient;
extern int board_id_hardcoding; // .ino 파일에 정의된 전역 변수 참조

// ====================================================================
// --- OTA 실행 함수 ---
// ====================================================================
void runHttpOtaSequence()
{
    uart0_printf("[OTA Task] Command received. Checking Wi-Fi...\n");

    if (WiFi.status() == WL_CONNECTED)
    {
        uart0_printf("[OTA Task] Wi-Fi is connected. Starting OTA process...\n");
        OTA_Firebase();
    }
    else
    {
        uart0_printf("[OTA Task] Error: Wi-Fi not connected. Cannot start OTA.\n");
    }
}

// ====================================================================
// --- 초기화 총괄 함수 ---
// ====================================================================
void setup_wifi_and_nvs()
{
    WiFi.onEvent(WiFiEvent);
    if (!connectToWiFiFromNVS())
    {
        setup_softap_launch(otaUpdateRequested);
        uart0_printf("[%8lu ms] [Setup] Entered SoftAP Configuration Mode.\n", millis());
    }
    else
    {
        prefs.begin(NVS_NAMESPACE, true);
        currentAppID = prefs.getString(NVS_KEY_APPID, DEFAULT_APPID);
        prefs.end();
        uart0_printf("[%8lu ms] [Setup] Loaded AppID: %s\n", millis(), currentAppID.c_str());
        storedTcpServerIp = loadTcpIpFromNVS();
    }
}

void setup_mqtt_client()
{
    if (root_ca == nullptr || root_ca[0] == '\0')
    {
        uart0_printf("[%8lu ms] [ERROR] Root CA certificate is empty or invalid!\n", millis());
    }
    clientSecure.setCACert(root_ca);
    client.setServer(MQTT_BROKER, MQTT_PORT);
    client.setCallback(callback);
}

void setup_tcp_client_task()
{
    void tcpReceiverTask(void *param);
    xTaskCreatePinnedToCore(tcpReceiverTask, "TCP_RX_Task", 4096, NULL, 1, NULL, 1);
}

// ====================================================================
// --- 시리얼 명령 처리 함수 (수정됨) ---
// ====================================================================
void handleSerialCommand()
{
    uint8_t data[128];                                         // 시리얼 데이터 수신 버퍼
    int len = uart_read_bytes(UART_NUM_0, data, (128 - 1), 0); // 0ms 타임아웃 (논블로킹)
    if (len > 0){
        data[len] = '\0'; // Null-terminate the received data
        for (int i = 0; i < len; i++){
            char inChar = (char)data[i];
            serialCommand += inChar; // 디버깅을 위해 수신된 문자열을 출력 (올바른 printf 포맷 사용)
            // uart0_printf("RX] %c (0x%02X)\n", inChar, inChar);
            if (inChar == '\n' || inChar == '\r'){
                serialCommand.trim();
                serialCommand.toLowerCase();
                uart0_printf("\n[CMD] '%s'", serialCommand.c_str()); // 수신된 전체 명령 출력
                if (serialCommand.equals("delete nvs")){
                    uart0_printf("[CMD] Clearing NVS and restarting...\n");
                    prefs.begin(NVS_NAMESPACE, false);
                    prefs.clear();
                    prefs.end();
                    ESP.restart();
                }
                else if (serialCommand.equals("ota firebase")){
                    uart0_printf("[CMD] OTA firebase command received. Will start after checking Wi-Fi.\n");
                    isOTACommand = true;
                }
                else if (serialCommand.equals("ota direct")){
                    uart0_printf("[CMD] OTA direct command received. Starting SoftAP for update...\n");
                    otaUpdateRequested = true;
                    shouldEnterSoftAP = true;
                }
                else if (serialCommand.startsWith("id=")){
                    String valueStr = serialCommand.substring(3);
                    int newId = valueStr.toInt();
                    if (valueStr.length() > 0 && newId >= 0 && newId <= 7){
                        board_id_hardcoding = newId;
                        saveBoardIdToNVS(newId);
                        uart0_printf("[CMD] Board ID set to: %d and saved to NVS.\n", board_id_hardcoding);
                    }
                    else{
                        uart0_printf("[CMD] Error: Invalid ID. Please use a number between 0 and 7.\n");
                    }
                }
                else if (serialCommand.equals("help")){
                    uart0_printf("\n--- Serial Command Help ---\n");
                    uart0_printf("ota firebase - Start OTA update from firebase server.\n");
                    uart0_printf("ota direct   - Start SoftAP for direct firmware upload.\n");
                    uart0_printf("delete nvs   - Clear all saved settings in internal NAND and restart.\n");
                    uart0_printf("id=<0-7>     - Set the board ID (e.g., id=3). !! doesn't work !! \n");
                    uart0_printf("help         - Show this help message.\n");
                    uart0_printf("--------------------------\n");
                }
                else{
                    uart0_printf("[CMD] Unknown command: '%s'\n", serialCommand.c_str());
                }

                serialCommand = ""; // 명령어 처리 후 초기화
            } // if (inChar == '\n' || inChar == '\r')
        } // for (int i = 0; i < len; i++)
    } // if (len > 0)
}

// ====================================================================
// --- 메인 Setup 및 Loop 함수 (실질적인 로직) ---
// ====================================================================
void setup_network_tasks()
{
    // init_uart0(); // UART 드라이버 직접 초기화
    // Serial.begin(SERIAL_BAUDRATE); // 더 이상 필요 없음
    delay(100);

    uart0_printf("[%8lu ms] --- ESP32-S3 MQTT/TCP Client (v2.4-DynamicID) ---\n", millis());
    board_id_hardcoding = loadBoardIdFromNVS(board_id_hardcoding);
    uart0_printf("[INFO] Current Board ID: %d\n", board_id_hardcoding);

    bootTimestamp = millis();

    setup_wifi_and_nvs();

    if (WiFi.status() == WL_CONNECTED)
    {
        uart0_printf("Configuring time from NTP server...\n");
        configTime(9 * 3600, 0, ntpServer);

        struct tm timeinfo;
        if (getLocalTime(&timeinfo))
        {
            uart0_printf("Time synchronized: %s", asctime(&timeinfo));
        }

        setup_mqtt_client();
        setup_tcp_client_task();
    }

    uart0_printf("[%8lu ms] [Setup] Main setup complete.\n", millis());
}

long loop_network_count = 0;
void loop_network_tasks()
{
    handleSerialCommand();

    if (isOTACommand)
    {
        runHttpOtaSequence();
        isOTACommand = false;
    }

    // Direct OTA 또는 일반 wifi 설정 모드 진입 처리
    if (shouldEnterSoftAP)
    {
        // otaUpdateRequested가 참이면 Direct OTA 모드로, 아니면 일반 wifi 설정 모드로 진입
        setup_softap_launch(otaUpdateRequested);
        while (shouldEnterSoftAP)
        { // 설정 모드 루프
            loop_config_mode();
            delay(1); // 다른 태스크에 시간 할당
        }
    }
    else
    {
        if (loop_network_count % 20 == 0)
        {
            loop_checkSafeTCP();
        }

        if (tcpClient.connected())
        {
            static unsigned long lastTcpSendAttempt_loop = 0;
            if (tcpSendInterval <= millis() - lastTcpSendAttempt_loop)
            {
                createAndSendTcpPacket();
                lastTcpSendAttempt_loop = millis();
            }
        }
    }
    // delay(10);

    loop_network_count++;
}

#endif // _NETWORK_TASK_H_
