/*
  <Arduino Config>
  BOARD : "ESP32-S3 DEVKITC-1 N8R2" (NOT 1.1) OR "ADAFRUIT QT py esp32-C3"
  DESC : SRAM 512K, ROM 384K, FLASH 8M, PSRAM 2M
  CONFIG : Flash QIO 80MHz, OPI PSRAM, Partition Minimal SPIFFS, others : apply default setting

  Pinmap for EZGEO : https://app.box.com/integrations/googledss/openGoogleEditor?fileId=1057800223831&trackingId=3&csrfToken=fec3c9e31a4e5bc1e73cc6f018a5d619fe6284085bd9bccd471d5a7ccdbf5a74#slide=id.g2e5f6fedfce_0_71

  SEE 74HC595 later - https://docs.arduino.cc/tutorials/communication/guide-to-shift-out
*/

/*
  #define DPINS_NUM 36
  int DPINS[DPINS_NUM] = { 4,5,6,7,15,    16,17,18,8,3,     46,9,10,11,12,    13,14,
                            43,44,1,2,42,  41,40,39,38,37,   36,35,0,45,48,    47,21,20,19};

  //  left pins   :  4,5,6,7,15,    16,17,18,8,3,     46,9,10,11,12,    13,14,
  //  right pins  :  43,44,1,2,42,  41,40,39,38,37,   36,35,0,45,48,    47,21,20,19
*/

#ifndef CONFIGPINS_H_
#define CONFIGPINS_H_
//-----------------------------------------------------
//  Active Mux index
//-----------------------------------------------------
#define MUX_LIST_LEN    9
int enList[MUX_LIST_LEN] = {0, 1, 2, 3, 4, 5, 6, 7, 8}; // ==> Simply En Jumper number list. From left to Right.

const int NUM_OF_MUX = MUX_LIST_LEN;
// int pinMuxEn[NUM_OF_MUX] = {41, 7, 40, 15, 39, 16, 38, 8, 37, 3, 36, 46, 35, 9, 0, 10, 45, 11, 47, 12, 21, 13}; // En0=41, En1=7, ... En21=13
int pinMuxEn[NUM_OF_MUX] = {41, 7, 40, 15, 39, 16, 38, 8, 37}; // En0=41, En1=7, ... En18=48

////////  HW Pin # - En of all mux
/*  IDK 답변
  1. GPIO33, 34, 35, 36, 37는 PSRAM 8MB형에서 사용 불가 합니다.  R2(2MB)혹은 PSRAM없는 형으로 사용하셔야 합니다.
  2. GPIO26은 PSRAM이 있으면 모두 사용 불가 합니다.
*/

//  Num of Mux output
#define MUX_74HC4067_OUT    16
#define MUX_74HC4051_OUT    8
#define NUM_MUX_OUT     (MUX_74HC4067_OUT)
#define NUM_USED_OUT        16       // only 5 pins are used

#define NUM_REAL_WIDTH   10
#define NUM_REAL_HEIGHT  12
#define NUM_REAL_SENSOR_CH  (NUM_REAL_WIDTH * NUM_REAL_HEIGHT)

int getEnListLen() { return MUX_LIST_LEN; };
int getEnPinById(int enIndex) { return enList[enIndex]; };
int active_mux_index = -1;

//  ADC Buffer
int adc_value[MUX_LIST_LEN][NUM_MUX_OUT];
int adc_ordered[MUX_LIST_LEN * NUM_MUX_OUT];
// int **adc_value;

//  HW Pin # = Signal 0~3 of all mux
const int NUM_MUX_SIG = 4;
int pinMuxSig[NUM_MUX_SIG] = {2, 5, 42, 6};
int S0, S1, S2, S3;

//  HW Pin # - Wakeup, ADC
int pinWakeup = 4;
int pinADC = A0;
#define RESOLUTION_BITS (8)  // choose resolution (explained in depth below)

//-----------------------------------------
//  DIP Switch
#define DIP_SW_NUM  10
bool dip_sw_value[DIP_SW_NUM];
bool dip_sw_value_last[DIP_SW_NUM];
int dip_decimal = 0;

//-----------------------------------------
//  RS485
int pin485U1DE = 19;


///////////////////////////////////////////////////////////////////////////
//    SETUP
///////////////////////////////////////////////////////////////////////////

void setup_HWPins_Grib() {
  pinMode(pin485U1DE, OUTPUT);
  digitalWrite(pin485U1DE, LOW); // LOW : RX, HIGH : TX

  analogReadResolution(RESOLUTION_BITS);  // set the resolution of analogRead results
                                          //  - maximum: 14 bits (maximum ADC resolution)
                                          //  - default: 10 bits (standard Arduino setting)
                                          //  - minimum:  1 bit

  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(pinADC, INPUT);

  for (int i = 0; i < NUM_OF_MUX; i++) {
    pinMode(pinMuxEn[i], OUTPUT);
  }

  for (int i = 0; i < NUM_MUX_SIG; i++) {
    pinMode(pinMuxSig[i], OUTPUT);
  }
  S0 = pinMuxSig[0];
  S1 = pinMuxSig[1];
  S2 = pinMuxSig[2];
  S3 = pinMuxSig[3];

  delay(5);
}

void updateDipSwVal(){
  dip_decimal = 0;

  for(int i = 0 ; i < DIP_SW_NUM ; i++) {
    dip_sw_value_last[i] = dip_sw_value[i];

    if(50 < adc_value[8][i]) // 50 : 임의의 유효값. 어차피 ON이면 200넘는 값이 뜬다.
      dip_sw_value[i] = true;
    else
      dip_sw_value[i] = false;

    dip_decimal |= (int(dip_sw_value[i]) << i);

    if(dip_sw_value_last[i] != dip_sw_value[i]) {
      Serial.printf("\nDIP SW [%d] is changed to %d {%d}\n", i, dip_sw_value[i], dip_decimal);
    }

  }

  
}

//-----------------------------------------------------
//    READ ADC WITH MUX
//-----------------------------------------------------
int loop_gpio_count = 0;

int muxSig4PinsVal[NUM_MUX_OUT][NUM_MUX_SIG] = {
    // [Mux Output : 0~15][Signal Pin : 0~3]
    //  {S0,S1,S2,S3}
    {0, 0, 0, 0},  // channel 0
    {1, 0, 0, 0},  // channel 1
    {0, 1, 0, 0},  // channel 2
    {1, 1, 0, 0},  // channel 3   It means S0=1, S1=1, S2=0, S3=0
    {0, 0, 1, 0},  // channel 4
    {1, 0, 1, 0},  // channel 5
    {0, 1, 1, 0},  // channel 6
    {1, 1, 1, 0},  // channel 7
#if NUM_MUX_OUT == MUX_74HC4067_OUT
    {0, 0, 0, 1},  // channel 8    It means S0=0, S1=0, S2=0, S3=1
    {1, 0, 0, 1},  // channel 9
    {0, 1, 0, 1},  // channel 10
    {1, 1, 0, 1},  // channel 11
    {0, 0, 1, 1},  // channel 12
    {1, 0, 1, 1},  // channel 13
    {0, 1, 1, 1},  // channel 14
    {1, 1, 1, 1}   // channel 15
#endif
};

/*
  USAGE : int value = read_1ch_in_mux(i); // i = 0~15
*/
int read_1ch_in_mux(int mux_ch) {
    // config signal 4 pins
    for (int i = 0; i < NUM_MUX_SIG; i++) {
        digitalWrite(pinMuxSig[i], muxSig4PinsVal[mux_ch][i]);
    }

    int adc_raw = analogRead(pinADC);

    return adc_raw;
}

/*
  16ch Mux IC 74HC4067 
  USAGE : int value[16]; read_16ch_in_mux(value); // value buffer will be filled.
*/
void read_16ch_in_mux(int* buf_16) {
    for (int i = 0; i < NUM_MUX_OUT; i++) {
        buf_16[i] = read_1ch_in_mux(i);
    }
}

//  8ch Mux IC 74HC4051
void read_8ch_in_mux(int* buf_16) {
    for (int i = 0; i < NUM_MUX_OUT ; i++) {
        buf_16[i] = read_1ch_in_mux(i);

        if(buf_16[i] <5)
          buf_16[i] = 0;
    }
}

/*
  //  Minimal pin change
  {0,0,0,0}, //channel 0

  {1,0,0,0}, //channel 1
  {1,1,0,0}, //channel 3   It means S0=1, S1=1, S2=0, S3=0
  {1,1,1,0}, //channel 7
  {1,1,1,1}  //channel 15

  {0,1,1,1}, //channel 14
  {0,0,1,1}, //channel 12
  {0,0,0,1}, //channel 8    It means S0=0, S1=0, S2=0, S3=1

  {1,0,0,1}, //channel 9
  {1,1,0,1}, //channel 11
  {0,1,0,1}, //channel 10
  {0,1,0,0}, //channel 2

  {0,1,1,0}, //channel 6
  {0,0,1,0}, //channel 4
  {1,0,1,0}, //channel 5
  {1,0,1,1}, //channel 13

*/
void read_16ch_in_mux_fast(int* buf_16) {
    //  ch 0, set all LOW
    for (int i = 0; i < NUM_MUX_SIG; i++) {
        digitalWrite(pinMuxSig[i], 0);
    }
    buf_16[0] = analogRead(pinADC);

    digitalWrite(S0, 1);  //  ch 1 : 1000
    buf_16[1] = analogRead(pinADC);
    digitalWrite(S1, 1);  //  ch 3 : 1100
    buf_16[3] = analogRead(pinADC);
    digitalWrite(S2, 1);  //  ch 7 : 1110
    buf_16[7] = analogRead(pinADC);
    digitalWrite(S3, 1);  //  ch 15 : 1111
    buf_16[15] = analogRead(pinADC);

    digitalWrite(S0, 0);  //  ch 14 : 0111
    buf_16[14] = analogRead(pinADC);
    digitalWrite(S1, 0);  //  ch 12 : 0011
    buf_16[12] = analogRead(pinADC);
    digitalWrite(S2, 0);  //  ch 8 : 0001
    buf_16[8] = analogRead(pinADC);

    digitalWrite(S0, 1);  //  ch 9 : 1001
    buf_16[9] = analogRead(pinADC);
    digitalWrite(S1, 1);  //  ch 11 : 1101
    buf_16[11] = analogRead(pinADC);
    digitalWrite(S0, 0);  //  ch 10 : 0101
    buf_16[10] = analogRead(pinADC);
    digitalWrite(S3, 0);  //  ch 2 : 0100
    buf_16[2] = analogRead(pinADC);

    digitalWrite(S2, 1);  //  ch 6 : 0110
    buf_16[6] = analogRead(pinADC);
    digitalWrite(S1, 0);  //  ch 4 : 0010
    buf_16[4] = analogRead(pinADC);
    digitalWrite(S0, 1);  //  ch 5 : 1010
    buf_16[5] = analogRead(pinADC);
    digitalWrite(S3, 1);  //  ch 13 : 1011
    buf_16[13] = analogRead(pinADC);
}


//-----------------------------------------------------
//    SELECT MUX
//-----------------------------------------------------
void selectMux(int mux_id) {
    for (int i = 0; i < NUM_OF_MUX; i++) {
        // for(int i = (NUM_OF_MUX - 1) ; 0 <= i ; i--) {
        digitalWrite(pinMuxEn[getEnPinById(i)], 1);
    }

    digitalWrite(pinMuxEn[getEnPinById(mux_id)], 0);

    active_mux_index = mux_id;
}

void changeMux(int mux_id) {
    digitalWrite(pinMuxEn[getEnPinById(active_mux_index)], 1);

    digitalWrite(pinMuxEn[getEnPinById(mux_id)], 0);

    active_mux_index = mux_id;
}

//======================================================================
//  reordering
void reorder_sensor_shape() {
  int line_buffer[MUX_LIST_LEN * NUM_MUX_OUT];

  for(int mux = 0 ; mux < 8 ; mux++) {
    for(int y_id = 0 ; y_id < 16 ; y_id++) {
      int counter = mux * 16 + y_id;
      line_buffer[counter] =  adc_value[mux][y_id];

      if(counter == NUM_REAL_SENSOR_CH)
        break;
    }
  }

  for(int i = 0 ; i < NUM_REAL_SENSOR_CH ; i++) {
    adc_ordered[i] =  line_buffer[NUM_REAL_SENSOR_CH - 1 - i];
  }

}

#endif // CONFIGPINS_H_