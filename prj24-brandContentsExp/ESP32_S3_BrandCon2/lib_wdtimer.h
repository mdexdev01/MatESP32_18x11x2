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
  timer = timerBegin(0, 80, true);                      // timer 0, div 80
  timerAttachInterrupt(timer, &onTimer, true);          // attach callback
  timerAlarmWrite(timer, MStoUM * TIMER_DUR_MS, true);  // set time in us
  timerAlarmEnable(timer);                              // enable interrupt
}


void loop_wdTimer() {
  // timerWrite(timer, 0);  // reset timer (feed watchdog)
  // long loopTime = millis();
  {
    // do something with timer
  }
  // loopTime = millis() - loopTime;
  // Serial.print("loop time is = ");
  // Serial.println(loopTime);  // should be under 3000

  if (timer_count_old != timer_count) {  // if(timer_count_old < timer_count) ==> it is normal. but it cannot protect overflow.
    timer_flag = true;
    timer_count_old = timer_count;
    // Serial.println("timer done");
  }
}

void changeTimerDurMS(int milliSecond) {
  timerAlarmWrite(timer, milliSecond * MStoUM, true);  // set time in us
  timerAlarmEnable(timer);                              // enable interrupt
}

void changeTimerDurUS(int microSecond) {
  timerAlarmWrite(timer, microSecond, true);  // set time in us
  timerAlarmEnable(timer);                              // enable interrupt
}
