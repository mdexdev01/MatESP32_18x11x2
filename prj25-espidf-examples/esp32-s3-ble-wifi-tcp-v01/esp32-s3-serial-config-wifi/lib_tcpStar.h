#include <WiFi.h>
#include <WiFiClient.h>

const char* ssid = "KT_GiGA_2G_Wave2_4587";
const char* password = "fab8ezx455";

const char* serverIP = "172.30.1.72";  // PC의 실제 IP ==> 이거 계속 바뀜. 이거 mqtt로 설정해줘야 함.
const uint16_t port = 9000;

WiFiClient client;

uint8_t binDip4 = 0;  // 보드 고유 ID (0~7)
const int PACKET_SIZE = 998 + 952;
uint8_t txBuffer[PACKET_SIZE];

unsigned long lastReconnectAttempt = 0;
const unsigned long reconnectInterval = 3000;
uint32_t sequenceNumber = 0;

TaskHandle_t receiverTaskHandle;

int printWifiStatus(int status);

void onWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_STA_CONNECTED:
      Serial.println("[WiFi] Connected to AP");
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("[WiFi] Disconnected from AP");
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.print("[WiFi] Got IP: ");
      Serial.println(WiFi.localIP());
      break;
    default:
      Serial.printf("[WiFi] Event: %d\n", event);
      break;
  }
}

void connectToServer() {
  if (client.connect(serverIP, port)) {
    Serial.println("Connected to server.");
  } else {
    Serial.println("Connection failed.");
  }
}

void receiverTask(void* param) {
  while (true) {
    if (client.connected() && client.available() >= PACKET_SIZE) {
      uint8_t rxBuf[PACKET_SIZE];
      client.read(rxBuf, PACKET_SIZE);

      if (rxBuf[0] == 0xFF && rxBuf[1] == 0xFF && rxBuf[PACKET_SIZE - 1] == 0xFE) {
        uint8_t seq = rxBuf[PACKET_SIZE - 2];
        Serial.printf("[RX] Packet received from PC | seq: %d\n", seq);
        // 필요한 처리를 여기에 추가 가능
      }
      else {
        Serial.printf("[RX] Packet Error");
      }
    }
    delay(1);
  }
}

void createAndSendPacket() {
  txBuffer[0] = 0xFF;
  txBuffer[1] = 0xFF;
  txBuffer[2] = 0x02;
  txBuffer[3] = binDip4;

  memset(txBuffer + 8, binDip4, PACKET_SIZE - 9);

  txBuffer[PACKET_SIZE - 2] = sequenceNumber % 100;
  txBuffer[PACKET_SIZE - 1] = 0xFE;

  client.write(txBuffer, PACKET_SIZE);
  if(sequenceNumber % 100 == 0)
    Serial.printf("seq=%d \n", sequenceNumber);

  sequenceNumber++;
}

bool isSSID2_4GHz(const char* target_ssid) {
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    String foundSSID = WiFi.SSID(i);
    if (foundSSID == target_ssid) {
      int freq = WiFi.channel(i);  // 채널 번호로 대역 판별
      if (freq >= 1 && freq <= 14) {
        Serial.printf("%s is 2.4GHz \n", target_ssid);
        return true;  // 2.4GHz
      } else {
        Serial.printf("%s is 5.0GHz \n", target_ssid);
        return false; // 5GHz (ESP32에서는 붙을 수 없음)
      }
    }
  }
  return false;  // SSID not found
}

void setup_tcpStar() {
  WiFi.onEvent(onWiFiEvent);
  
  if(false == isSSID2_4GHz(ssid)) {
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
    Serial.printf("\n[OK] WiFi connected!, IP: %s ==> let's go\n", WiFi.localIP().toString().c_str());
  }

  connectToServer();
}

void loop_tcpStar(){
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    delay(1000);
    Serial.print(".");
    return;
  }

  if (!client.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt >= reconnectInterval) {
      Serial.println("Attempting reconnect...");
      client.stop();
      connectToServer();
      lastReconnectAttempt = now;
    }
    return;
  }

  createAndSendPacket();
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

  Serial.printf("%s \n", logText);
  return status;
}
