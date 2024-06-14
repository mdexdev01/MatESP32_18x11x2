#include "esp_system.h"

const int MStoUM = 1000;  // time in ms to trigger the watchdog

hw_timer_t* timer = NULL;
int isrCountTimer = 0;
long storedCount = 0;

int tickTimerDur = 0; 
long tickTimerElapsed = 0;
long tickTimerBegin = 0;


void ARDUINO_ISR_ATTR onTimer() {
    isrCountTimer++;

    // ets_printf("isr[wdt] %d, (%d) \n", isrCountTimer, millis());
}

void turnOnTimerMS(long duration) { //  MS : Milli second
  timerAlarmWrite(timer, MStoUM * duration, true);  // set time in us
  timerAlarmEnable(timer);                          // enable interrupt

  tickTimerBegin = millis();
  tickTimerDur = duration;
  tickTimerElapsed = 0;
  storedCount = 0;
}

void turnOffTimer() {
  timerWrite(timer, 0);  // reset timer (feed watchdog)
}

void changeTimerDurMS(int milliSecond) { //  MS : Milli second
  timerAlarmWrite(timer, milliSecond * MStoUM, true);  // set time in us
  
  long stored_dur = tickTimerDur;
  tickTimerDur = milliSecond;
  ets_printf(">> timer duration change = %d MS to %d MS \n", stored_dur, milliSecond);
}

void changeTimerDurUS(int microSecond) {  // US : Micro second
  if(microSecond < 10) // 10 is the minimum at the clock 80 div mode.
    microSecond = 10; 
  timerAlarmWrite(timer, microSecond, true);  // set time in us

  tickTimerDur = microSecond;

  ets_printf(">> timer duration change = %d US \n", microSecond);
}

long getElapsedTick() {
  return tickTimerElapsed;
}


void setup_wdTimer() {
  // Serial.begin(230400);

  // pinMode(button, INPUT_PULLUP);                    // init control pin
  timer = timerBegin(0, 80, true);                      // timer 0, div 80MHz (default for ESP32-S3)
  timerAttachInterrupt(timer, &onTimer, true);          // attach callback
}

void loop_wdTimer() {
  // timerWrite(timer, 0);  // reset timer (feed watchdog)
  tickTimerElapsed = millis() - tickTimerBegin;

  if(storedCount < isrCountTimer) {
    ets_printf("timer duration = %d, timer count = %d, (%d.%d sec) \n", tickTimerDur, isrCountTimer, tickTimerElapsed / 1000, tickTimerElapsed % 1000);
    storedCount < isrCountTimer;
  }

}

