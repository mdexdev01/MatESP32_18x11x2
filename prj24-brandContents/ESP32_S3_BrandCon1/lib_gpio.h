#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>  // Required for 16 MHz Adafruit Trinket
#endif

#include "libLED_Object.h"
#include "lib_wdtimer.h"


#define pin_LED_WS2812C 48
Adafruit_NeoPixel strip0;

void setup_neopixel();
void setNeopixelColor(int pinNeopixel, int r, int g, int b);

#define SW_1 13
#define SW_2 14

struct Button {
  const uint8_t PIN;
  char nickName[16];
  bool changed;
  bool isClicked;
  bool isClickedLong;
  bool isClickedVeryLong;
  
  uint32_t numberKeyPresses;
  uint32_t timeClicked_MS;
};

Button button1 = { SW_1, "REST", false, false, false, false, 0, 0 };
Button button2 = { SW_2, "FUNC", false, false, false, false, 0, 0 };

void checkTimeLen(Button *theButton);

void ARDUINO_ISR_ATTR isr1(void* arg) {
  Button* s = static_cast<Button*>(arg);
  s->numberKeyPresses += 1;
  s->changed = true;
  // ets_printf("isr[1] %d\n", s->numberKeyPresses);
}

void ARDUINO_ISR_ATTR isr2(void* arg) {
  Button* s = static_cast<Button*>(arg);
  button2.numberKeyPresses += 1;
  button2.changed = true;
  ets_printf("isr[2] %d, %s \n", s->numberKeyPresses, s->nickName);
}


void setup_gpioWork() {
  //  TACT BUTTONS...
  pinMode(button1.PIN, INPUT_PULLUP);
  attachInterruptArg((button1.PIN), isr1, &button1, CHANGE);
  button1.isClicked = 1 - digitalRead(SW_1);

  pinMode(button2.PIN, INPUT_PULLUP);
  attachInterruptArg((button2.PIN), isr2, &button2, CHANGE);
  button2.isClicked = 1 - digitalRead(SW_2);

  //  LEDs...
  pinMode(pin_LED_WS2812C, OUTPUT);
  setup_neopixel();
}

//  pin_LED_WS2812C
void setup_neopixel() {
  strip0.setPin(pin_LED_WS2812C);
  strip0.updateType(NEO_GRB + NEO_KHZ800);
  strip0.updateLength(1);
  // strip[i].updateLength(LED_COUNT);

  strip0.begin();            // INITIALIZE NeoPixel strip object (REQUIRED)
  strip0.setBrightness(20);  // Set BRIGHTNESS to about 1/5 (max = 255)

  //  LED Show 
  {
    int delay_term = 200;
    setNeopixelColor(pin_LED_WS2812C, 255, 0, 0); delay(delay_term);
    setNeopixelColor(pin_LED_WS2812C, 0, 255, 0); delay(delay_term);
    setNeopixelColor(pin_LED_WS2812C, 0, 0, 255); delay(delay_term);

    setNeopixelColor(pin_LED_WS2812C, 255, 255, 0); delay(delay_term);
    setNeopixelColor(pin_LED_WS2812C, 0, 255, 255); delay(delay_term);
    setNeopixelColor(pin_LED_WS2812C, 255, 0, 255); delay(delay_term);

    setNeopixelColor(pin_LED_WS2812C, 255, 255, 255); delay(delay_term);

    setNeopixelColor(pin_LED_WS2812C, 100, 50, 0);
  }

}

void setNeopixelColor(int pinNeopixel, int r, int g, int b) {
  switch(pinNeopixel) {
  case pin_LED_WS2812C:
    strip0.setPixelColor(0, strip0.Color(r, g, b));
    strip0.show();  // Turn OFF all pixels ASAP
    break;
  }

}


void loop_gpioWork() {
  if (button1.changed) {
    button1.isClicked = 1 - digitalRead(SW_1);  // click : 0(LOW), unclick : 1(HIGH)
    button1.timeClicked_MS = millis();

    // ets_printf("button[1] %d\n", button1.isClicked);
    button1.changed = false;
    if(button1.isClicked == false){
      button1.isClickedVeryLong = false;
      button1.isClickedLong = false;
      setNeopixelColor(pin_LED_WS2812C, 100, 50, 0);
    }
  }
  if (button2.changed) {                     // BUG : config.h 에서 초기에 이쪽으로 타고 들어옴. 핀을 OUTPUT으로 바꾸는듯.
    bool is_active = 1 - digitalRead(SW_2);  // click : 0(LOW), unclick : 1(HIGH)
    button2.isClicked = is_active;
    button2.timeClicked_MS = millis();

    Serial.printf("Button 2 (%d) has been changed %u times\n", is_active, button2.numberKeyPresses);
    button2.changed = false;
    if (is_active) {
      // Serial.printf("button 2 active\n");
    } else {
      // Serial.printf("button 2 inactive\n");
    }
  }

  checkTimeLen(&button1);
  checkTimeLen(&button2);
}

void checkTimeLen(Button * theButton) {
  if(theButton->isClicked) {
    int time_len = millis() - theButton->timeClicked_MS;
    if((3000 < time_len) && (theButton->isClickedVeryLong == false)){
      theButton->isClickedVeryLong = true;
      setNeopixelColor(pin_LED_WS2812C, 50, 250, 50);
      // Serial.printf("%s clicked - VERY LONG (%d ms) \n", theButton->nickName, time_len);
      delay(200);

      Serial.printf("RESTART \n");
      ESP.restart();
    }
    else if ((1000 < time_len) && (theButton->isClickedLong == false) ) {
      theButton->isClickedLong = true;
      setNeopixelColor(pin_LED_WS2812C, 0, 50, 200);
      // Serial.printf("%s clicked - LONG (%d ms) \n", theButton->nickName, time_len);
    }
  }

}
