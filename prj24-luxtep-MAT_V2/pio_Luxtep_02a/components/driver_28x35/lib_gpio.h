#ifndef LIB_GPIO_H_
#define LIB_GPIO_H_

// #include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#include "configPins-mdll-24-6822.h"
#include "libLED_Object.h"
#include "lib_wdtimer.h"

#define pin_LED_WS2812C 100
// Adafruit_NeoPixel strip0;

void setup_buzzer();
void setup_neopixel();
void setNeopixelColor(int pinNeopixel, int r, int g, int b);

#define SW_0 45
#define SW_1 48
#define SW_2 16

struct Button {
    const uint8_t PIN;
    char nickName[16];
    bool needsUpdate;
    bool isClicked;
    bool isClickedLong;
    bool isClickedVeryLong;

    uint32_t numberKeyPressed;
    uint32_t timeClicked_MS;
};

Button button0 = {SW_0, "SW-0", false, false, false, false, 0, 0};
Button button1 = {SW_1, "SW-1", false, false, false, false, 0, 0};
Button button2 = {SW_2, "SW-2", false, false, false, false, 0, 0};

#define TACT_NUM 3
Button* TactButtons[TACT_NUM];  // = {button0, button1, button2};

void checkTimeLen(Button* theButton);

void ARDUINO_ISR_ATTR isr_Buttons(void* arg) {
    Button* s = static_cast<Button*>(arg);
    s->numberKeyPressed += 1;
    s->needsUpdate = true;
    // ets_printf("isr %s\n", s->nickName);
}

void ARDUINO_ISR_ATTR isr0(void* arg) {
    Button* s = static_cast<Button*>(arg);
    s->numberKeyPressed += 1;
    s->needsUpdate = true;
    // button0.numberKeyPressed += 1;
    // button0.needsUpdate = true;
    // ets_printf("isr[0] %d\n", s->numberKeyPressed);
}

void setup_gpioWork() {
    //  TACT BUTTONS...
    int i = 0;
    TactButtons[i++] = &button0;
    TactButtons[i++] = &button1;
    TactButtons[i++] = &button2;

    for (int j = 0; j < TACT_NUM; j++) {
        Button* pTact = TactButtons[j];
        pinMode(pTact->PIN, INPUT_PULLUP);
        attachInterruptArg((pTact->PIN), isr_Buttons, pTact, CHANGE);
        pTact->isClicked = 1 - digitalRead(pTact->PIN);
    }

    //  LEDs...
    // pinMode(pin_LED_WS2812C, OUTPUT);
    // setup_neopixel();
}

//  pin_LED_WS2812C
// void setup_neopixel() {
//     strip0.setPin(pin_LED_WS2812C);  // ????????????????
//     strip0.updateType(NEO_GRB + NEO_KHZ800);
//     strip0.updateLength(1);
//     // strip[i].updateLength(LED_COUNT);

//     strip0.begin();            // INITIALIZE NeoPixel strip object (REQUIRED)
//     strip0.setBrightness(20);  // Set BRIGHTNESS to about 1/5 (max = 255)

//     //  LED Show
//     {
//         int delay_term = 200;
//         setNeopixelColor(pin_LED_WS2812C, 255, 0, 0);
//         delay(delay_term);
//         setNeopixelColor(pin_LED_WS2812C, 0, 255, 0);
//         delay(delay_term);
//         setNeopixelColor(pin_LED_WS2812C, 0, 0, 255);
//         delay(delay_term);

//         setNeopixelColor(pin_LED_WS2812C, 255, 255, 0);
//         delay(delay_term);
//         setNeopixelColor(pin_LED_WS2812C, 0, 255, 255);
//         delay(delay_term);
//         setNeopixelColor(pin_LED_WS2812C, 255, 0, 255);
//         delay(delay_term);

//         setNeopixelColor(pin_LED_WS2812C, 255, 255, 255);
//         delay(delay_term);

//         setNeopixelColor(pin_LED_WS2812C, 100, 50, 0);
//     }
// }

extern int indi_1_r;
extern int indi_1_g;
extern int indi_1_b;

void setNeopixelColor(int pinNeopixel, int r, int g, int b) {
    indi_1_r = r;
    indi_1_g = g;
    indi_1_b = b;

    // switch (pinNeopixel) {
    // }
}

void loop_gpioWork() {
    for (int i = 0; i < TACT_NUM; i++) {
        Button* pTact = TactButtons[i];
        checkTimeLen(pTact);

        if (pTact->needsUpdate == false)
            continue;

        pTact->isClicked = 1 - digitalRead(pTact->PIN);
        pTact->needsUpdate = false;
        pTact->timeClicked_MS = millis();

        uart0_printf("{%8d}Tact[%s] (T/F): %d, %d times \n",
                     pTact->timeClicked_MS, pTact->nickName, pTact->isClicked, pTact->numberKeyPressed);
        if (pTact->isClicked == false) {
            pTact->isClickedLong = false;
            pTact->isClickedVeryLong = false;
        } else {
        }
    }
}

void loop_gpioWork_old() {
    if (button0.needsUpdate) {
        button0.isClicked = 1 - digitalRead(SW_0);  // click : 0(LOW), unclick : 1(HIGH)
        button0.timeClicked_MS = millis();

        uart0_printf("button[0] %d\n", button0.isClicked);
        button0.needsUpdate = false;
        if (button0.isClicked == false) {
            button0.isClickedVeryLong = false;
            button0.isClickedLong = false;
            setNeopixelColor(pin_LED_WS2812C, 100, 50, 0);
        }
    }

    if (button1.needsUpdate) {
        button1.isClicked = 1 - digitalRead(SW_1);  // click : 0(LOW), unclick : 1(HIGH)
        button1.timeClicked_MS = millis();

        uart0_printf("button[1] %d\n", button1.isClicked);
        button1.needsUpdate = false;
        if (button1.isClicked == false) {
            button1.isClickedVeryLong = false;
            button1.isClickedLong = false;
            setNeopixelColor(pin_LED_WS2812C, 100, 50, 0);
        }
    }

    if (button2.needsUpdate) {                      // BUG : config.h 에서 초기에 이쪽으로 타고 들어옴. 핀을 OUTPUT으로 바꾸는듯.
        button2.isClicked = 1 - digitalRead(SW_2);  // click : 0(LOW), unclick : 1(HIGH)
        button2.timeClicked_MS = millis();

        uart0_printf("button [2] (%d) needsUpdate %u times\n", button2.isClicked, button2.numberKeyPressed);
        button2.needsUpdate = false;
        if (button2.isClicked) {
            uart0_printf("button 2 active\n");
        } else {
            uart0_printf("button 2 inactive\n");
        }
    }

    checkTimeLen(&button0);
    checkTimeLen(&button1);
    checkTimeLen(&button2);
}

void checkTimeLen(Button* theButton) {
    if (theButton->isClicked) {
        int time_len = millis() - theButton->timeClicked_MS;

        if ((3000 < time_len) && (theButton->isClickedVeryLong == false)) {
            theButton->isClickedVeryLong = true;
            setNeopixelColor(pin_LED_WS2812C, 50, 250, 250);
            uart0_printf("[%s] clicked - VERY LONG (%d ms) \n", theButton->nickName, time_len);
            delay(200);

            uart0_printf("RESTART \n");
            // ESP.restart();
        } else if ((1000 < time_len) && (theButton->isClickedLong == false)) {
            theButton->isClickedLong = true;
            setNeopixelColor(pin_LED_WS2812C, 0, 250, 0);
            uart0_printf("[%s] clicked - LONG (%d ms) \n", theButton->nickName, time_len);
        }
    }
}

#endif  // LIB_GPIO_H_