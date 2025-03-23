// -------------------------------------------------
// Copyright (c) 2022 HiBit <https://www.hibit.dev>
// -------------------------------------------------

#include "pitches.h"

#define pin_BZ1 7
#define pin_BZ2 8

int melody[] = {
    // REST, NOTE_E5, NOTE_D5, NOTE_C5, NOTE_B4, NOTE_A4, NOTE_G4, NOTE_A4, NOTE_B4,
    NOTE_G5, NOTE_E5,  // NOTE_F5, NOTE_G5, NOTE_E5, NOTE_F5, NOTE_G5, REST,
    // NOTE_E5, NOTE_C5, NOTE_D5, NOTE_E5, NOTE_C5, NOTE_D5, NOTE_E5, NOTE_C5, NOTE_D5, NOTE_E5, NOTE_D5, NOTE_C5,
    // NOTE_A4, NOTE_A4, REST, NOTE_A4, NOTE_A4, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_G4, REST, NOTE_G4,
    // NOTE_A4, NOTE_A4, NOTE_A4, NOTE_A4, NOTE_C5, NOTE_B4,
    REST};

int durations[] = {
    // 4, 2, 2, 2, 2, 2, 2, 2, 4,
    4, 8,  // 8, 4, 8, 8, 2, 2,
    // 4, 8, 8, 4, 8, 8, 4, 8, 8, 4, 8, 8,
    // 4, 8, 8, 4, 8, 8, 8, 8, 2, 8, 8,
    // 8, 8, 4, 4, 4, 1,
    1};

void setup_song() {
    pinMode(pin_BZ1, OUTPUT);
    pinMode(pin_BZ2, OUTPUT);
}

int loopcount_buzzer = 0;
void loop_buzzer() {
    //  tone examples : https://www.hibit.dev/posts/62/playing-popular-songs-with-arduino-and-a-buzzer
    int num = loopcount_buzzer % 20;

    if ((num % 2) == 0) {
        // analogWrite(pin_BZ1, 1000);
        tone(pin_BZ1, 1000 * num, 50);
    } else {
        // analogWrite(pin_BZ2, 10000);
        // tone(pin_BZ2, 1000 * num, 1000);
    }

    loopcount_buzzer++;
}

//  Many songs here
//  https://github.com/hibit-dev/buzzer/tree/master/src/songs
void play_song() {
    int size = sizeof(durations) / sizeof(int);

    for (int note = 0; note < size; note++) {
        // to calculate the note duration, take one second divided by the note type.
        // e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
        int duration = 1000 / durations[note];
        tone(pin_BZ1, melody[note], duration);

        // to distinguish the notes, set a minimum time between them.
        // the note's duration + 30% seems to work well:
        int pauseBetweenNotes = duration * 1.30;
        delay(pauseBetweenNotes);

        // stop the tone playing:
        noTone(pin_BZ1);
    }
}