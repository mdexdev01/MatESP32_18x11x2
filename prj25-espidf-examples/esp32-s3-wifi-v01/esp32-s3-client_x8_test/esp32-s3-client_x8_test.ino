#include "lib_tcpStar.h"


void setup() {
  Serial.begin(921600);
  delay(500);

  setup_tcpStar();

  xTaskCreatePinnedToCore(receiverTask, "RXTask", 4096, NULL, 1, &receiverTaskHandle, 1);
}

void loop() {
  loop_tcpStar();
  delay(10);  // 20Hz
}
