#include "esp_system.h"

int TIMER_DUR_MS = 250;   // 250ms
const int MStoUM = 1000;  // time in ms to trigger the watchdog
hw_timer_t* timer = NULL;

int timer_count = 0;
int timer_count_old = 0;
boolean timer_flag = true;

void ARDUINO_ISR_ATTR onTimer() {
    timer_count++;

    // ets_printf("isr[wdt] %d, (%d) \n", timer_count, millis());
}

void setup_wdTimer() {
    // Serial.begin(230400);

    // pinMode(button, INPUT_PULLUP);                    // init control pin
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

    if(timer_count_old < timer_count) {
      timer_flag = true;
      timer_count_old = timer_count;
    }
}

void changeTimerDurMS(int milliSecond) {
    timerAlarmWrite(timer, milliSecond * MStoUM, true);  // set time in us
}

void changeTimerDurUS(int microSecond) {
    timerAlarmWrite(timer, microSecond, true);  // set time in us
}
