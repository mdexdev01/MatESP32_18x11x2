#ifndef _LIB_WIFI_CONFIG_H_
#define _LIB_WIFI_CONFIG_H_
#include "libPrintRaw.h"
#include <Preferences.h>
#include <WiFiClient.h>

Preferences gPrefrences;
const char* MY_SOFTAP_SSID = "LUXTEP_mat_main";

bool onWifiSetting = false;  // Wi-Fi 설정 모드인지 여부

// const char* AP_SSID = "KT_GiGA_2G_Wave2_4587";
// const char* AP_PASSWORD = "fab8ezx455";

// const char* TCP_APP_SERVER_IP = "172.30.1.72";  // PC의 실제 IP ==> 이거 계속 바뀜. 이거 mqtt로 설정해줘야 함.
const uint16_t TCP_APP_PORT = 9000;

WiFiClient * pTcpClient = NULL;

extern int binDip4;  // 보드 고유 ID (0~7)
const int PACKET_SIZE = 998 + 952;
uint8_t txBuffer[PACKET_SIZE];

unsigned long lastReconnectAttempt = 0;
const unsigned long reconnectInterval = 3000; // 3000 milliseconds
uint32_t sequenceNumber = 0;

TaskHandle_t receiverTaskHandle;

boolean isTCP_Connected = false;  // TCP 연결 상태

int printWifiStatus(int status);


void onWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_STA_CONNECTED:
      uart0_printf("[WiFi] Connected to AP \n");
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      uart0_printf("[WiFi] Disconnected from AP \n");
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      uart0_printf("[WiFi] Got IP: %s \n", WiFi.localIP().toString().c_str());
      break;
    default:
      uart0_printf("[WiFi] Event: %d\n", event);
      break;
  }
}


bool isSSID2_4GHz(const char* target_ssid) {
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    String foundSSID = WiFi.SSID(i);
    if (foundSSID == target_ssid) {
      int freq = WiFi.channel(i);  // 채널 번호로 대역 판별
      if (freq >= 1 && freq <= 14) {
        uart0_printf("%s is 2.4GHz \n", target_ssid);
        return true;  // 2.4GHz
      } else {
        uart0_printf("%s is 5.0GHz \n", target_ssid);
        return false; // 5GHz (ESP32에서는 붙을 수 없음)
      }
    }
  }
  return false;  // SSID not found
}

bool connectToWiFi() {
    gPrefrences.begin("wifi", true);
    String ssid = gPrefrences.getString("ssid", "");
    String password = gPrefrences.getString("password", "");
    String server_ip = gPrefrences.getString("server_ip", "");
    gPrefrences.end();

    uart0_printf("Connect to WiFi SSID: %s, Password: %s, Server IP: %s\n", ssid.c_str(), password.c_str(), server_ip.c_str());
    if (ssid == "") return false;

    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        delay(500);
        uart0_printf(".");
    }
    uart0_printf("Wait done\n");
    return WiFi.status() == WL_CONNECTED;
}

void connectToServer() {
    gPrefrences.begin("wifi", true);
    String server_ip = gPrefrences.getString("server_ip", "");
    gPrefrences.end();

    if(pTcpClient == NULL) {
        pTcpClient = new WiFiClient();
    }

    if (pTcpClient->connect(server_ip.c_str(), TCP_APP_PORT)) {
        uart0_printf("TCP Connected to server. (%s) \n", server_ip.c_str());
        isTCP_Connected = true;  // 연결 상태 초기화
    } else {
        uart0_printf("TCP Connection failed. (%s) \n", server_ip.c_str());
        isTCP_Connected = false;  // 연결 상태 초기화
    }
}

void sendPacketTCP(const uint8_t* tx_packet, size_t packet_size) {
    if (pTcpClient && pTcpClient->connected()) {
        pTcpClient->write(tx_packet, packet_size);
    } else {
        uart0_printf("TCP Client not connected. Cannot send packet.\n");
    }
}

void buildAndSendPacket_Test() {
  txBuffer[0] = 0xFF;
  txBuffer[1] = 0xFF;
  txBuffer[2] = 0x02;
  txBuffer[3] = binDip4;

  memset(txBuffer + 8, binDip4, PACKET_SIZE - 9);

  txBuffer[PACKET_SIZE - 2] = sequenceNumber % 100;
  txBuffer[PACKET_SIZE - 1] = 0xFE;

  pTcpClient->write(txBuffer, PACKET_SIZE);
  if(sequenceNumber % 100 == 0)
    uart0_printf("seq=%d \n", sequenceNumber);

  sequenceNumber++;
}

void setup_tcpStar() {
    gPrefrences.begin("wifi", true);
    String ssid = gPrefrences.getString("ssid", "");
    String password = gPrefrences.getString("password", "");
    String server_ip = gPrefrences.getString("server_ip", "");
    gPrefrences.end();

    uart0_printf("Setup WiFi SSID: %s, Password: %s, Server IP: %s\n", ssid.c_str(), password.c_str(), server_ip.c_str());

    WiFi.onEvent(onWiFiEvent);

    if(ssid == "" || server_ip == "") {
        uart0_printf("WiFi credentials not found in preferences.\n");
        return;
    }
    
    if(false == isSSID2_4GHz(ssid.c_str())) {
        printWifiStatus(WiFi.status());
        return;
    }
    WiFi.begin(ssid, password);

    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
        printWifiStatus(WiFi.status());
        delay(500);
    }

    if (WiFi.status() != WL_CONNECTED) {
        printWifiStatus(WiFi.status());
    } else {
        uart0_printf("\n[OK] WiFi connected!, IP: %s ==> let's go\n", WiFi.localIP().toString().c_str());
    }

    connectToServer();
}


void loop_tcpStar(){
    // int64_t cur_snap = esp_timer_get_time();  // 마이크로초 단위로 타이머 초기화
    // int64_t duration_us = esp_timer_get_time() - cur_snap;
    // uart0_printf("wifi status DUR %5lld us) \n", duration_us);

    int wifiStatus = WiFi.status(); // 1~7us
    if (wifiStatus!= WL_CONNECTED) {
        WiFi.reconnect();
        delay(100);
        uart0_printf(".");
        return;
    }

    //  TCP RECONNECT LOGIC
    if (false == pTcpClient->connected()) { // 18~140us. typically 18us or 70us
        isTCP_Connected = false;  // 연결 상태 초기화

        unsigned long now = millis();

        gPrefrences.begin("wifi", true);
        String server_ip = gPrefrences.getString("server_ip", "");
        gPrefrences.end();

        if (reconnectInterval <= now - lastReconnectAttempt) {
          uart0_printf("Attempting reconnect to %s :%d... \n", server_ip.c_str(), TCP_APP_PORT);
          pTcpClient->stop();
          connectToServer();
          lastReconnectAttempt = now;
        }
        return;
    }

//   buildAndSendPacket_Test();
}

char logText[80];
int printWifiStatus(int status) {
  switch(status) {
    case WL_IDLE_STATUS:
      sprintf(logText, "(0)WL_IDLE_STATUS, Initial state");
      break;
    case WL_NO_SSID_AVAIL:
      sprintf(logText, "(1)WL_NO_SSID_AVAIL. Ex) can't find SSID. Check Large/Small character");
      break;
    case WL_SCAN_COMPLETED:
      sprintf(logText, "(2)WL_SCAN_COMPLETED");
      break;
    case WL_CONNECTED:
      sprintf(logText, "(3)WL_CONNECTED. My IP = %s", WiFi.localIP().toString().c_str());
      break;
    case WL_CONNECT_FAILED:
      sprintf(logText, "(4)WL_CONNECT_FAILED. Ex) Wrong pass, Mac ban %s", WiFi.macAddress().c_str());
      break;
    case WL_CONNECTION_LOST:
      sprintf(logText, "(5)WL_CONNECTION_LOST Ex) Remote has disconnected to me");
      break;
    case WL_DISCONNECTED:
      sprintf(logText, "(6)WL_DISCONNECTED Ex) I disconnected. Or Before tyring to connect");
      break;
    default:
      sprintf(logText, "(wifi status %d)", status);
      break;
  }

  uart0_printf("%s \n", logText);
  return status;
}

#endif  // _LIB_WIFI_CONFIG_H_