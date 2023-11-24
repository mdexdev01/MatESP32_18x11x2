#include <Arduino.h>

#include "lib_wdtimer.h"

#define LED_R 14
#define LED_G 21
#define LED_B 47
#define SW_1 45
#define SW_2 48
#define SW_DIP 12

struct Button {
    const uint8_t PIN;
    char nickName[16];
    uint32_t numberKeyPresses;
    bool changed;
};

Button button1 = {SW_1, "REST", 0, false};
Button button2 = {SW_2, "FUNC", 0, false};
Button button3 = {SW_DIP, "DIP_M0S1", 0, false};

void ARDUINO_ISR_ATTR isr1(void* arg) {
    Button* s = static_cast<Button*>(arg);
    s->numberKeyPresses += 1;
    s->changed = true;
    // ets_printf("gpio isr[1] %d\n", s->numberKeyPresses);
}

void ARDUINO_ISR_ATTR isr2(void* arg) {
    Button* s = static_cast<Button*>(arg);
    button2.numberKeyPresses += 1;
    button2.changed = true;
    ets_printf("gpio isr[2] %d\n", s->numberKeyPresses);
}

void ARDUINO_ISR_ATTR isr3() {
    button3.numberKeyPresses += 1;
    button3.changed = true;
    ets_printf("gpio isr[3]\n");
}

bool ledB_state = false;

void set_led_state() {
    if (ledB_state == true) {
        digitalWrite(LED_B, LOW);  // LED 켜기
    } else {
        digitalWrite(LED_B, HIGH);  // LED 끄기
    }
}

void setup_gpioWork() {
    //  TACT BUTTONS...
    pinMode(button1.PIN, INPUT_PULLUP);
    attachInterruptArg((button1.PIN), isr1, &button1, CHANGE);

    pinMode(button2.PIN, INPUT_PULLUP);
    attachInterruptArg((button2.PIN), isr2, &button2, CHANGE);

    //  DIP SWITCH...
    pinMode(button3.PIN, INPUT_PULLUP);
    attachInterrupt((button3.PIN), isr3, CHANGE);

    //  LEDs...
    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);

    digitalWrite(LED_R, HIGH);
    digitalWrite(LED_G, HIGH);
    digitalWrite(LED_B, HIGH);
}

void loop_gpioWork() {
    if (button1.changed) {
        bool is_active = 1 - digitalRead(SW_1);  // click : 0(LOW), unclick : 1(HIGH)
        button1.changed = false;
        if (is_active) {
            // esp_restart();
        } else {
        }
    }
    if (button2.changed) {                       // BUG : config.h 에서 초기에 이쪽으로 타고 들어옴. 핀을 OUTPUT으로 바꾸는듯.
        bool is_active = 1 - digitalRead(SW_2);  // click : 0(LOW), unclick : 1(HIGH)
        Serial.printf("Button 2 (%d) has been changed %u times\n", is_active, button2.numberKeyPresses);
        button2.changed = false;
        if (is_active) {
            ledB_state = true;
            Serial.printf("button 2 active\n");
        } else {
            ledB_state = false;
            Serial.printf("button 2 inactive\n");
        }
    }
    if (button3.changed) {
        bool is_active = 1 - digitalRead(SW_DIP);  // click : 0(LOW), unclick : 1(HIGH)
        button3.changed = false;
        if (is_active) {
            Serial.println("button 3 clicked");
        } else {
            Serial.println("button 3 unclicked");
        }
    }
    // set_led_state();
}

/*
//	NOT USE
void setup_ledtest() {
    // put your setup code here, to run once:
    pinMode(SW_1, INPUT);
    pinMode(SW_2, INPUT);
    pinMode(SW_DIP, INPUT);

    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);

    digitalWrite(LED_R, HIGH);
    digitalWrite(LED_G, HIGH);
    digitalWrite(LED_B, HIGH);

    Serial.begin(230400);
    // Serial1.begin(115200);
    // Serial.begin(921600);
    // Serial1.begin(921600, SERIAL_8N1, 18, 17);
}

void loop_ledtest() {
    // put your main code here, to run repeatedly:
    Serial.println("\nSW Testing...");  // display a message
    int switch_1 = digitalRead(SW_1);
    if (switch_1 == LOW) {
        digitalWrite(LED_R, LOW);  // LED 켜기
        Serial.println("SW_1 ON, LED_R 켜짐");
        Serial.println("\n -- rebooting -- ");
        ESP.restart();

    } else {
        digitalWrite(LED_R, HIGH);  // LED 끄기
        Serial.println("SW_1 OFF, LED_R 꺼짐");
    }

    int switch_2 = digitalRead(SW_2);
    if (switch_2 == LOW) {
        digitalWrite(LED_G, LOW);  // LED 켜기
        Serial.println("SW_2 ON, LED_G 켜짐");
    } else {
        digitalWrite(LED_G, HIGH);  // LED 끄기
        Serial.println("SW_2 OFF, LED_G 꺼짐");
    }

    int switch_dip = digitalRead(SW_DIP);
    if (switch_dip == LOW) {
        digitalWrite(LED_B, LOW);  // LED 켜기
        Serial.println("SW_DIP ON, LED_B 켜짐");
    } else {
        digitalWrite(LED_B, HIGH);  // LED 끄기
        Serial.println("SW_DIP OFF, LED_B 꺼짐");
    }
}
*/
