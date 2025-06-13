#include <WiFi.h>
#include <Preferences.h>
#include <ArduinoJson.h>

#define BAUD_RATE 921600
#define MAX_PACKET_SIZE 256
#define HEADER1 0xFF
#define HEADER2 0xFF
#define FOOTER 0xFE

#define ACK_OK   0x01
#define ACK_FAIL 0x00

Preferences prefs;

void setup() {
  Serial.begin(BAUD_RATE);
  WiFi.mode(WIFI_STA);
  Serial.println("ESP32 Ready");
}

void loop() {
  if (Serial.available() < 8) {
    return;
  }

  uint8_t buffer[MAX_PACKET_SIZE];
  Serial.readBytes(buffer, 8);

  if (buffer[0] != HEADER1 || buffer[1] != HEADER2) return;

  uint8_t totalLen = buffer[6] * 100 + buffer[7] + 8;
  if (totalLen > MAX_PACKET_SIZE || totalLen < 9) return;

  int remaining = totalLen - 8;
  int bytesRead = Serial.readBytes(buffer + 8, remaining);
  if (bytesRead != remaining) return;
  if (buffer[totalLen - 1] != FOOTER) return;

  // JSON 추출
  int jsonStart = 8;
  int jsonLength = totalLen - 8 - 2;
  buffer[totalLen - 1] = '\0';  // JSON 종료 처리

  char jsonStr[jsonLength + 1];
  memcpy(jsonStr, &buffer[jsonStart], jsonLength);
  jsonStr[jsonLength] = '\0';

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, jsonStr);
  if (err) {
    Serial.println("JSON parsing failed");
    sendAck(false);
    return;
  }

  String ssid = doc["ssid"] | "";
  String pass = doc["pass"] | "";
  String ip   = doc["ip"] | "";

  Serial.printf("Received SSID: %s\n", ssid.c_str());
  Serial.printf("Received PASS: %s\n", pass.c_str());
  Serial.printf("Server IP: %s\n", ip.c_str());

  if (ssid.length() == 0 || ip.length() == 0) {
    sendAck(false);
    return;
  }


  // Wi-Fi 연결 시도
  WiFi.begin(ssid.c_str(), pass.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts++ < 20) {
    Serial.printf("~%d~", attempts);
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi connected!");
    sendAck(true);

    // 저장
    prefs.begin("wifi", false);
    prefs.putString("ssid", ssid);
    prefs.putString("password", pass);
    prefs.putString("server_ip", ip);
    prefs.end();
  } else {
    Serial.println("Wi-Fi connect failed.");
    sendAck(false);
  }

}

void sendAck(bool success) {
  uint8_t ackPacket[10] = {
    HEADER1, HEADER2,
    0x02, // board_id or source_id
    0x02, // packet type: ACK
    0x00, 0x00,
    10,   // total length
    success ? ACK_OK : ACK_FAIL,
    0x00, // reserved
    FOOTER
  };

  Serial.write(ackPacket, 10);
  // Serial.printf("ACK sent: %s\n", success ? "OK" : "FAIL");
}
