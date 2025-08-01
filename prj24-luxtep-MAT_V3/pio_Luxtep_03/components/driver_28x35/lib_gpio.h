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

#define pin_TACT0 45
#define pin_TACT1 48
#define pin_TACT2 16

struct Button {
    const uint8_t PIN;
    char nickName[16];
    bool isUpdateISR;
    bool isClicked;
    bool isClickedLong;
    bool isClickedVeryLong;

    uint32_t numberKeyPressed;
    uint32_t timeClicked_MS;
    uint32_t timeClickElapsed_MS;
};

Button button0 = {pin_TACT0, "Silk)SW-1.Blur", false, false, false, false, 0, 0, 0};
Button button1 = {pin_TACT1, "Silk)SW-2.Snap", false, false, false, false, 0, 0, 0};
Button button2 = {pin_TACT2, "Silk)SW-3.Air" , false, false, false, false, 0, 0, 0};

#define TACT_NUM 3
Button* TactButtons[TACT_NUM];  // = {button0, button1, button2};

int stateButton1 = 0;
bool btnUnNoticed = false;

void checkTimeLen(Button* theButton);

void ARDUINO_ISR_ATTR isr_Buttons(void* arg) {
    Button* s = static_cast<Button*>(arg);
    s->isUpdateISR = true;
    // s->numberKeyPressed += 1;
    // ets_printf("isr %s\n", s->nickName);
}

void onRelease(Button* theButton) {
    theButton->numberKeyPressed += 1;

    stateButton1 = (TactButtons[1]->numberKeyPressed % 3);

    btnUnNoticed = true;
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

}


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

        if (pTact->isUpdateISR == false)
            continue;

        pTact->isClicked = 1 - digitalRead(pTact->PIN);
        pTact->isUpdateISR = false;

        if(pTact->isClicked == true) {
            pTact->numberKeyPressed += 1;

            pTact->timeClicked_MS = millis();
            pTact->timeClickElapsed_MS = 0; // initialize
            pTact->isClickedLong = false;
            pTact->isClickedVeryLong = false;

            setNeopixelColor(pin_LED_WS2812C, 20, 20, 20);
        } else {
            pTact->timeClickElapsed_MS = millis() - pTact->timeClicked_MS;

            setNeopixelColor(pin_LED_WS2812C, 0, 0, 0);
        }

        uart0_printf("\n[%8d]Tact(%s) {T/F}:%d, %d times \n",
                     millis(), pTact->nickName, pTact->isClicked, pTact->numberKeyPressed);

    }
}


#define LONG_CLICK_MS 1000
#define VERY_LONG_CLICK_MS 3000

void checkTimeLen(Button* theButton) {
    if (theButton->isClicked == false)
        return;

    int time_len = millis() - theButton->timeClicked_MS;
    theButton->timeClickElapsed_MS = time_len;

    if ((VERY_LONG_CLICK_MS < time_len) && (theButton->isClickedVeryLong == false)) {
        theButton->isClickedVeryLong = true;
        setNeopixelColor(pin_LED_WS2812C, 80, 30, 0);
        uart0_printf("[%s] clicked - VERY LONG (%d ms) \n", theButton->nickName, time_len);

        uart0_printf("RESTART \n");
        // delay(200);
        // ESP.restart();
    } else if ((LONG_CLICK_MS < time_len) && (theButton->isClickedLong == false)) {
        theButton->isClickedLong = true;
        setNeopixelColor(pin_LED_WS2812C, 0, 60, 60);
        uart0_printf("[%s] clicked - LONG (%d ms) \n", theButton->nickName, time_len);
    }

    return;
}

#endif  // LIB_GPIO_H_