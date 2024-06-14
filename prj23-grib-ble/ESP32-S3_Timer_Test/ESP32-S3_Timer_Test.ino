/*
  ESP32 S3 Timer
  Author : Marveldex Inc. area@mdex.co.kr
  Date : 20240524
*/

#include "lib_wdtimer.h"

void setup() {
  // put your setup code here, to run once:
  Serial.printf("SETUP-WD TIMER \n");

  setup_wdTimer();
  turnOnTimerMS(500);   // 500ms
}

void loop() {
  // put your main code here, to run repeatedly:
  loop_wdTimer();

  //  change duration example
  long runtime = getElapsedTick();
  if(12000 < runtime && runtime < 13000) {
    changeTimerDurMS(1000); // milli second
  }
  else if(10000 < runtime && runtime < 11000) {
    changeTimerDurUS(20); // micro second
  }

  delay(400);

}
