#include "esp_system.h"

int TIMER_DUR_MS = 40;    // ms
const int MStoUM = 1000;  // time in ms to trigger the watchdog
hw_timer_t* timer = NULL;

ulong timer_count = 0;
ulong timer_count_old = 0;
boolean timer_flag = true;

void ARDUINO_ISR_ATTR onTimer() {
  timer_count++;
  // ets_printf("isr[wdt] %d, (%d) \n", timer_count, millis());
}

void setup_wdTimer() {
  //  timer esp32 bsp V.3.0
  timer = timerBegin(1000000);                     //timer 1Mhz resolution
  timerAttachInterrupt(timer, &onTimer);       //attach callback

  timerAlarm(timer, TIMER_DUR_MS * MStoUM, true, 0);  //set time in us

}


void loop_wdTimer() {
  if (timer_count_old != timer_count) {  // if(timer_count_old < timer_count) ==> it is normal. but it cannot protect overflow.
    timer_flag = true;
    timer_count_old = timer_count;
    // Serial.println("timer done");
  }
}

/*
void changeTimerDurMS(int milliSecond) {
  timerAlarmWrite(timer, milliSecond * MStoUM, true);  // set time in us
  timerAlarmEnable(timer);                              // enable interrupt
}

void changeTimerDurUS(int microSecond) {
  timerAlarmWrite(timer, microSecond, true);  // set time in us
  timerAlarmEnable(timer);                              // enable interrupt
}
*/