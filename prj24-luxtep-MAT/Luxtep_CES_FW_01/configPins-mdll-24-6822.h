#ifndef CONFIGPINS_H_
#define CONFIGPINS_H_
#include <Arduino.h>
#include <driver/adc.h>
#include <driver/gpio.h>
//   rs485 - rxd, txd, de

//   dip switch - opt 0, 1 [GPIO]
//   OPT0 : MAIN OR SUB
//   OPT1 : NONE

//   dip switch - A 0, 1, 2, 3 [GPIO]
//   SWA0 :

//  LED indicator

//----------------------------------------------------
//  MDLL-24-6822 Pinout
//----------------------------------------------------
//  4067 selection pins
//  Num of Mux output
#define MUX_74HC4067_OUT 16
#define MUX_74HC4067_HALF_OUT 8
#define MUX_74HC4051_OUT 8

#define NUM_MUX_OUT MUX_74HC4051_OUT

#define NUM_SUB_WIDTH 28
#define NUM_SUB_HEIGHT 34
#define NUM_SUB_SENSOR_CH (NUM_SUB_WIDTH * NUM_SUB_HEIGHT)

//-------------------------------------------------------------
//   MUX, POWER[28] - NX3L4051HR power  [S0 ~ S2, Z, EN0~EN3] : EXT0~EXT27,
//  Mux for Ext power - NX3L4051HR
const int NUM_MUX_SIG = 3;
#define pinExtS0 4
#define pinExtS1 5
#define pinExtS2 6
int pinMuxPwrSig012[NUM_MUX_SIG] = {pinExtS0, pinExtS1, pinExtS2};

const int NUM_PWR_MUX = 4;
#define pinExtEn0 20
#define pinExtEn1 3
#define pinExtEn2 46
#define pinExtEn3 9
int pinMuxPwrEn[NUM_PWR_MUX] = {pinExtEn0, pinExtEn1, pinExtEn2, pinExtEn3};
int en_ext_mux_id = -1;

//-------------------------------------------------------------
//   MUX, INPUT[34]  - NX3L4051HR input  [AN0~AN2, Z, EN4~EN8] : SEN0~SEN33,, SW-OPT0, 1,, SW-A0~A3
//  Mux for Sensor - NX3L4051HR
#define pinSenS0 42  //  AN0
#define pinSenS1 41  //  AN1
#define pinSenS2 40  //  AN2
int pinMuxSenSig012[NUM_MUX_SIG] = {pinSenS0, pinSenS1, pinSenS2};

const int NUM_SEN_MUX = 5;
#define pinSenEn0 10
#define pinSenEn1 11
#define pinSenEn2 12
#define pinSenEn3 39
#define pinSenEn4 38
int pinMuxSenEn[NUM_SEN_MUX] = {pinSenEn0, pinSenEn1, pinSenEn2, pinSenEn3, pinSenEn4};
int en_sen_mux_id = -1;

//-------------------------------------------------------------
//   adc - aref [ADC]
//   adc - aout [ADC]
// #define pinVOut 1  //  ADC1_0
// #define pinVRef 2  //  ADC1_1
#define pinVOut ADC1_CHANNEL_0  // GPIO 1
#define pinVRef ADC1_CHANNEL_1  // GPIO 2

//  ADC Buffer
#define NUM_PWR 28  // X
#define NUM_SEN 34  // Y

#define NUM_COL NUM_PWR  // X, 28
#define NUM_ROW NUM_SEN  // Y, 34

#define SIZE_X NUM_COL
#define SIZE_Y NUM_ROW

int VOut_Value[NUM_PWR][NUM_SEN];
int VRef_Value[NUM_PWR][NUM_SEN];
int ForceData[NUM_PWR][NUM_SEN];

byte force_buf[SIZE_X * SIZE_Y];
byte inpol_buf[SIZE_X * (SIZE_Y + 1)];  // interpolation buffer

byte force_buf_1[SIZE_X * SIZE_Y];
byte inpol_buf_1[SIZE_X * (SIZE_Y + 1)];  // interpolation buffer

//  Dip Switch
int dip_decimal = 0;  // 0: Main, 1: Sub

//  HW Pin # - Wakeup, ADC
#define RESOLUTION_BITS (12)  // choose resolution (explained in depth below)

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
//------------------------------------
//  DIP SWITCH
#define NUM_OF_DIP 6
boolean dip_state[NUM_OF_DIP];

//-------------------------------------------------------------
//   TACT - 1, 2, 3 [GPIO]

//-------------------------------------------------------------
//   BUZZER 1, 2 [PWM]
#define BZ1_CTL 7
#define BZ2_CTL 15

//-------------------------------------------------------------
//  LED strip - DI0~DI3
// #define DI0 47
#define DI0 8
#define DI1 21
#define DI2 14
#define DI3 13

///////////////////////////////////////////////////////////////////////////
//    SETUP
///////////////////////////////////////////////////////////////////////////
void setup_HWPins_34x28() {
    analogReadResolution(RESOLUTION_BITS);  // set the resolution of analogRead results
                                            //  - maximum: 14 bits (maximum ADC resolution)
                                            //  - default: 10 bits (standard Arduino setting)
                                            //  - minimum:  1 bit

    // pinMode(LED_BUILTIN, OUTPUT);

    // LED matrix
    pinMode(DI0, OUTPUT);
    pinMode(DI1, OUTPUT);
    pinMode(DI2, OUTPUT);
    pinMode(DI3, OUTPUT);

    pinMode(pinVOut, INPUT);
    pinMode(pinVRef, INPUT);

    adc1_config_channel_atten(pinVOut, ADC_ATTEN_DB_11);  // 감쇠 설정
    adc1_config_channel_atten(pinVRef, ADC_ATTEN_DB_11);

    for (int i = 0; i < NUM_MUX_SIG; i++) {
        pinMode(pinMuxPwrSig012[i], OUTPUT);
        digitalWrite(pinMuxPwrSig012[i], LOW);
        // pinMode(pinMuxLatSel[i], OUTPUT);
        // digitalWrite(pinMuxLatSel[i], LOW);
        pinMode(pinMuxSenSig012[i], OUTPUT);
        digitalWrite(pinMuxSenSig012[i], LOW);
    }

    for (int i = 0; i < NUM_PWR_MUX; i++) {
        pinMode(pinMuxPwrEn[i], OUTPUT);
        digitalWrite(pinMuxPwrEn[i], HIGH);
    }

    for (int i = 0; i < NUM_SEN_MUX; i++) {
        pinMode(pinMuxSenEn[i], OUTPUT);
        digitalWrite(pinMuxSenEn[i], HIGH);
    }

    delay(1);
}

//-----------------------------------------------------
//    READ ADC WITH MUX
//-----------------------------------------------------
int loop_gpio_count = 0;

int muxSig3PinsVal[NUM_MUX_OUT][NUM_MUX_SIG] = {
    // [Mux Output : 0~7][Signal Pin : 0~2]
    //  {S0,S1,S2}
    {0, 0, 0},  // channel 0
    {1, 0, 0},  // channel 1
    {0, 1, 0},  // channel 2
    {1, 1, 0},  // channel 3   It means S0=1, S1=1, S2=0
    {0, 0, 1},  // channel 4
    {1, 0, 1},  // channel 5
    {0, 1, 1},  // channel 6
    {1, 1, 1},  // channel 7
};

//-----------------------------------------------------
//    MUX WORKING - EXT
//-----------------------------------------------------
void selectPwrMux(int ext_mux_id) {
    if (en_ext_mux_id == ext_mux_id)
        return;

    // for (int i = 0; i < NUM_PWR_MUX; i++) {
    //     digitalWrite(pinMuxPwrEn[i], HIGH);
    // }

    digitalWrite(pinMuxPwrEn[en_ext_mux_id], HIGH);
    digitalWrite(pinMuxPwrEn[ext_mux_id], LOW);

    en_ext_mux_id = ext_mux_id;
}

void selectPwrOut(int ext_ch) {
    // config signal 4 pins
    for (int i = 0; i < NUM_MUX_SIG; i++) {
        digitalWrite(pinMuxPwrSig012[i], muxSig3PinsVal[ext_ch][i]);
    }
}

void selectPwrCh(int ext_ch) {
    int mux_id = ext_ch / MUX_74HC4051_OUT;
    int mux_out = ext_ch % MUX_74HC4051_OUT;
    selectPwrMux(mux_id);
    selectPwrOut(mux_out);
}

//-----------------------------------------------------
//    MUX WORKING - SENSOR
//-----------------------------------------------------
const int NOISE_THRESHOLD = 4;

int calcR(int vout, int vref) {
    // float cur_ref = (float)vref / 1000;  // /1000 = (1/100)/10
    int sensor_R = 0;
    if (vref != 0)
        sensor_R = vout * 1000 / vref;  // /1000 = (1/100)/10
    return sensor_R;
}

int calcF(int vout, int vref) {
    int value_r = calcR(vout, vref);
    int force = 0;
    const int ZERO_ALIGN = 25;
    if (value_r != 0) {
        //	x/24 : 1 , x/2 : 240
        force = 600 * 1000 / value_r;  // 800 * 1000 : arbitrary value, value_r: 24000 ~ 2000

        force -= ZERO_ALIGN;
        if (force <= 3) {  // align zero point
            force = 0;
        }
        if (239 <= force)
            force = 239;
    } else
        return -1;

    return force;
}

void selectSenMux(int sen_mux_id) {
    if (en_sen_mux_id == sen_mux_id)
        return;

    // for (int i = 0; i < NUM_SEN_MUX; i++) {
    //     digitalWrite(pinMuxSenEn[i], HIGH);
    // }

    digitalWrite(pinMuxSenEn[en_sen_mux_id], HIGH);  // old
    digitalWrite(pinMuxSenEn[sen_mux_id], LOW);      // new

    en_sen_mux_id = sen_mux_id;
}

int _vout_raw;
int _vref_raw;
int read1chInSenMux(int mux_ch) {
    // config signal 4 pins
    for (int i = 0; i < NUM_MUX_SIG; i++) {
        digitalWrite(pinMuxSenSig012[i], muxSig3PinsVal[mux_ch][i]);
    }

    _vout_raw = adc1_get_raw(pinVOut);
    _vref_raw = adc1_get_raw(pinVRef);

    return _vout_raw;
}

void read8chInSenMux(int sen_ch) {
    for (int i = 0; i < NUM_MUX_OUT; i++) {
        read1chInSenMux(i);
        VOut_Value[sen_ch][i] = _vout_raw;
        VRef_Value[sen_ch][i] = _vref_raw;
    }
}

void readSen8ch_fast(int sen_ch) {
    for (int i = 0; i < NUM_MUX_OUT; i++) {
        read1chInSenMux(i);
        VOut_Value[sen_ch][i] = _vout_raw;
        VRef_Value[sen_ch][i] = _vref_raw;
    }
}

void readAndCalc(int col_index, int mux_id, int mux_ch, int pin_signal) {
    digitalWrite(pinMuxSenSig012[pin_signal], muxSig3PinsVal[mux_ch][pin_signal]);
    // if (muxSig3PinsVal[mux_ch][pin_signal] == 1)
    //     GPIO.out_w1ts = (1 << pinMuxSenSig012[pin_signal]);  // PIN을 HIGH로 설정
    // else
    //     GPIO.out_w1tc = (1 << pinMuxSenSig012[pin_signal]);  // PIN을 LOW로 설정

    int row_index = mux_id * NUM_MUX_OUT + mux_ch;

    // delayMicroseconds(200);adc1_get_raw

    // VOut_Value[col_index][row_index] = _vout_raw = analogRead(pinVOut);
    // VRef_Value[col_index][row_index] = _vref_raw = analogRead(pinVRef);
    VOut_Value[col_index][row_index] = _vout_raw = adc1_get_raw(pinVOut);
    VRef_Value[col_index][row_index] = _vref_raw = adc1_get_raw(pinVRef);

    int calc_value = calcF(_vout_raw, _vref_raw);
    // force_value = 0;
    if (calc_value < NOISE_THRESHOLD)
        ForceData[col_index][row_index] = 0;
    else
        ForceData[col_index][row_index] = calc_value;
}

void readSen1Mux(int col_index, int mux_id) {
    int mux_ch = 0;
    for (int i = 0; i < NUM_MUX_SIG; i++) {
        digitalWrite(pinMuxSenSig012[i], muxSig3PinsVal[mux_ch][i]);
    }

    // {0, 0, 0},  // channel 0
    readAndCalc(col_index, mux_id, 0, 0);
    // {1, 0, 0},  // channel 1
    readAndCalc(col_index, mux_id, 1, 0);
    // {1, 1, 0},  // channel 3   It means S0=1, S1=1, S2=0
    readAndCalc(col_index, mux_id, 3, 1);
    // {0, 1, 0},  // channel 2
    readAndCalc(col_index, mux_id, 2, 0);

    // {0, 1, 1},  // channel 6
    readAndCalc(col_index, mux_id, 6, 2);
    // {1, 1, 1},  // channel 7
    readAndCalc(col_index, mux_id, 7, 0);
    // {1, 0, 1},  // channel 5
    readAndCalc(col_index, mux_id, 5, 1);
    // {0, 0, 1},  // channel 4
    readAndCalc(col_index, mux_id, 4, 0);
}

void readSen1Col(int col_index) {
    int num_mux = NUM_ROW / NUM_MUX_OUT;  // 34 / 8 = 4
    for (int mux_id = 0; mux_id < num_mux; mux_id++) {
        // Serial.printf("muxid : %d (/%d) \n", mux_id, num_mux);
        selectSenMux(mux_id);
        readSen1Mux(col_index, mux_id);
    }

    selectSenMux(4);
    for (int i = 0; i < NUM_MUX_SIG; i++) {
        digitalWrite(pinMuxSenSig012[i], muxSig3PinsVal[0][i]);  // out : 0, sigpin : i
    }
    readAndCalc(col_index, 4, 0, 0);  // mux id : 4, out : 0, sigpin : 0
    readAndCalc(col_index, 4, 1, 0);  // mux id : 4, out : 1, sigpin : 0
}

void readSenCh(int sen_ch) {
    int mux_id = sen_ch / MUX_74HC4051_OUT;
    int mux_out = sen_ch % MUX_74HC4051_OUT;
    selectSenMux(mux_id);
    read1chInSenMux(mux_out);
}

//-----------------------------------------------------
//    MUX WORKING - DIP SW
//-----------------------------------------------------
int binDip4 = 0;
void read_dipsw() {
    // choose mux_sen #4, ch34~39
    int dip_val[NUM_OF_DIP];

    //  _vref_raw : low is under 150 (typically 135), high is over 500 (typically 522)

    readSenCh(36);
    dip_val[0] = _vref_raw;
    readSenCh(37);
    dip_val[1] = _vref_raw;
    readSenCh(38);
    dip_val[2] = _vref_raw;
    readSenCh(39);
    dip_val[3] = _vref_raw;
    readSenCh(34);
    dip_val[4] = _vref_raw;
    readSenCh(35);
    dip_val[5] = _vref_raw;

    for (int i = 0; i < NUM_OF_DIP; i++) {
        if (300 < dip_val[i])
            dip_state[i] = HIGH;
        else
            dip_state[i] = LOW;
    }
    // Serial.printf("DIP [0]%d [1]%d [2]%d [3]%d [4]%d [5]%d\n", dip_val[0], dip_val[1], dip_val[2], dip_val[3], dip_val[4], dip_val[5]);
    // Serial.printf("DIP [0]%d [1]%d [2]%d [3]%d [4]%d [5]%d\n", dip_state[0], dip_state[1], dip_state[2], dip_state[3], dip_state[4], dip_state[5]);

    binDip4 = 0;
    for (int i = 0; i < 4; i++) {
        binDip4 |= (dip_state[i] << i);
    }
}

#endif  // CONFIGPINS_H_
