#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <Update.h>
#include "lib_ota.h"

void setup() {
  Serial.begin(921600);
  // pinMode(BUTTON_PIN, INPUT);
  // pinMode(LED_PIN, OUTPUT);
  // digitalWrite(LED_PIN, LOW);
  delay(2000);
}

void loop() {
  delay(500);
  isOTACommand = true;

  loop_ota();
}


