#ifndef _GROUP_APP_COMMAND_H_
#define _GROUP_APP_COMMAND_H_

#include <ArduinoJson.h>
#include "libProtocol.h"
#include "packetBuffer.h"

extern int MY_BOARD_ID;
extern int indexPermit;

bool tryToConnectWifiAP(String ssid_ap, String pass_ap);


int temp_reser_1 = 0;
int buildPacket_WifiApConfigAck(byte *tx_packet_buffer, int wifi_ap_config_ack) {
    tx_packet_buffer[IDX_HEADER_0] = HEADER_SYNC;   // 0xFF
    tx_packet_buffer[IDX_HEADER_1] = HEADER_SYNC;   // 0xFF
    tx_packet_buffer[IDX_VER] = 2;        // Major Ver = 2 (2025-02-20)
    tx_packet_buffer[IDX_TX_BOARD_ID] = M_BOARD_0;  // 

    tx_packet_buffer[IDX_GROUP_ID] = G_APP_COMMAND;     // GROUP ID
    tx_packet_buffer[IDX_MSG_ID] = M_AC_WIFI_AP_CONFIG_INFO_ACK;  // MSG ID
    tx_packet_buffer[IDX_DATA_0] = byte(wifi_ap_config_ack & 0xFF);     // ack, 1: OK, 0: FAIL
    tx_packet_buffer[IDX_DATA_1] = 0;                   // Reserved 1

    int pa_index = HEAD_LEN;

    tx_packet_buffer[pa_index++] = 0;          //  Reserved 2
    tx_packet_buffer[pa_index++] = TAIL_SYNC;  // 0xFE

    temp_reser_1++;

    return pa_index;
}



int parsePacket_WifiApConfigInfo(byte *rx_packet_header, byte *rx_packet_body, int rx_packet_body_len) {

    if (rx_packet_header[IDX_MSG_ID] != M_AC_WIFI_AP_CONFIG_INFO) {
        return -1;  // Invalid message ID
    }
/*

    int json_len = rx_packet_body_len;
    int remaining = json_len;


    // JSON 추출
    int jsonStart = 0;
    int jsonLength = json_len - TAIL_LEN;  // Exclude tail length

    char jsonStr[jsonLength + 1];
    memcpy(jsonStr, &rx_packet_body[jsonStart], jsonLength);
    jsonStr[jsonLength] = '\0';

    uart0_printf("JSON : %s \n", jsonStr);

    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, jsonStr);
    if (err) {
        uart0_printf("JSON parsing failed\n");
        // sendAck(false);
        return -2;
    }

    String ssid = doc["ssid"] | "";
    String pass = doc["pass"] | "";
    String ip   = doc["ip"] | "";

    uart0_printf("[JSON:SSID]: %s\n", ssid.c_str());
    uart0_printf("[JSON:PASS]: %s\n", pass.c_str());
    uart0_printf("[JSON:Server IP]: %s\n", ip.c_str());

    if (ssid.length() == 0 || ip.length() == 0) {
        uart0_printf("[JSON] no ssid, ip\n");
        return -3;
    }

    // 저장
    // gPrefrences.begin("wifi", false);
    // gPrefrences.putString("ssid", ssid);
    // gPrefrences.putString("password", pass);
    // gPrefrences.putString("server_ip", ip);
    // gPrefrences.end();

    uart0_printf("[JSON] Save Pref, SSID: %s, Password: %s, Server IP: %s\n", 
        ssid.c_str(), pass.c_str(), ip.c_str());



    // Try to connect to WiFi
    // Wi-Fi 연결 시도
    if(false == tryToConnectWifiAP(ssid, pass)) {
        uart0_printf("[JSON] Failed to connect to Wi-Fi AP: %s\n", ssid.c_str());
        // sendAck(false);
        return -4;  // Connection failed
    }

    if (WiFi.status() != WL_CONNECTED) {
        uart0_printf("[JSON] Wi-Fi connect failed. SSID: %s, Password: %s, Server IP: %s\n", 
            ssid.c_str(), pass.c_str(), ip.c_str());
        return -5;
    }

    // Save to preferences
    uart0_printf("[JSON] Wi-Fi connected!\n");
    // sendAck(true);

    pTcpClient->stop();  // 기존 TCP 연결 종료
    delete pTcpClient;
    pTcpClient = new WiFiClient();  // 새로운 TCP 클라이언트 생성
    uart0_printf("[JSON] TCP Client created.\n");

    // TCP 서버에 연결 시도
    
    if (pTcpClient->connect(ip.c_str(), TCP_APP_PORT)) {
        uart0_printf("[JSON] TCP Connected to server. (%s) \n", ip.c_str());
    } else {
        uart0_printf("[JSON] TCP Connection failed. (%s) \n", ip.c_str());
        return -6;
    }

    // sendAck(false);
*/

    return 0;
}

bool tryToConnectWifiAP(String ssid_ap, String pass_ap) {
/*    
    if(false == isSSID2_4GHz(ssid_ap.c_str())) {
        printWifiStatus(WiFi.status());
        return false;
    }

    // Wi-Fi 연결 시도
    WiFi.begin(ssid_ap.c_str(), pass_ap.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts++ < 20) {
        Serial.printf("~%d~", attempts);
        delay(500);
    }

    if (WiFi.status() != WL_CONNECTED) {
        return false;  // 실패
    }
*/
    return true;
}

#endif  // _GROUP_APP_COMMAND_H_