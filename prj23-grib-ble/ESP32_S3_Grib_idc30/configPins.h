/*
  <Arduino Config>
  BOARD : "ESP32-S3 DEVKITC-1" (NOT 1.1) OR "ADAFRUIT QT py esp32-C3"
  DESC : SRAM 512K, ROM 384K, FLASH 16M, PSRAM 8M  
  CONFIG : Flash QIO 80MHz, OPI PSRAM, Partition Minimal SPIFFS, others : apply default setting

  SEE 74HC595 later - https://docs.arduino.cc/tutorials/communication/guide-to-shift-out
*/

/*
  #define DPINS_NUM 36
  int DPINS[DPINS_NUM] = { 4,5,6,7,15,    16,17,18,8,3,     46,9,10,11,12,    13,14, 
                            43,44,1,2,42,  41,40,39,38,37,   36,35,0,45,48,    47,21,20,19};

  //  left pins   :  4,5,6,7,15,    16,17,18,8,3,     46,9,10,11,12,    13,14,
  //  right pins  :  43,44,1,2,42,  41,40,39,38,37,   36,35,0,45,48,    47,21,20,19
*/

//-----------------------------------------------------
//  Active Mux index
//-----------------------------------------------------
#define MUX_LIST_LEN    18
// int enList[MUX_LIST_LEN] = {14, 21, 9}; // ==> Simply En Jumper number list. From left to Right.
int enList[MUX_LIST_LEN] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17}; // ==> Simply En Jumper number list. From left to Right.


//  HW Pin # - En of all mux
const int NUM_OF_MUX = 22;
int pinMuxEn[NUM_OF_MUX] = {41, 7, 40, 15, 39, 16, 38, 8, 37, 3, 36, 46, 35, 9, 0, 10, 45, 11, 47, 12, 21, 13}; // En0=41, En1=7, ... En21=13

//  Num of Mux output
#define NUM_MUX_OUT     16


int getEnListLen() { return MUX_LIST_LEN; };
int getEnPinById(int enIndex) { return enList[enIndex]; };
int active_mux_index = -1;

//  ADC Buffer
int adc_value[MUX_LIST_LEN][NUM_MUX_OUT];
// int **adc_value;


//  HW Pin # = Signal 0~3 of all mux
const int NUM_MUX_SIG = 4;
int pinMuxSig[NUM_MUX_SIG] = {2, 5, 42, 6 };
int S0, S1, S2, S3;

//  HW Pin # - Wakeup, ADC
int pinWakeup = 4;
int pinADC = A0;
#define RESOLUTION_BITS (8)      // choose resolution (explained in depth below)


///////////////////////////////////////////////////////////////////////////
//    SETUP
///////////////////////////////////////////////////////////////////////////

void setup_HWPins_Grib() {
  analogReadResolution(RESOLUTION_BITS);    // set the resolution of analogRead results
                                            //  - maximum: 14 bits (maximum ADC resolution)
                                            //  - default: 10 bits (standard Arduino setting)
                                            //  - minimum:  1 bit

  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(pinADC, INPUT);    

  for ( int i = 0 ; i < NUM_OF_MUX ; i++) {
    pinMode(pinMuxEn[i], OUTPUT);    
  }

  for ( int i = 0 ; i < NUM_MUX_SIG ; i++) {
    pinMode(pinMuxSig[i], OUTPUT);    
  }
  S0 = pinMuxSig[0];
  S1 = pinMuxSig[1];
  S2 = pinMuxSig[2];
  S3 = pinMuxSig[3];

}


//-----------------------------------------------------
//    READ ADC WITH MUX
//-----------------------------------------------------
int loop_gpio_count = 0;


int muxSig4PinsVal[NUM_MUX_OUT][NUM_MUX_SIG]={  // [Mux Output : 0~15][Signal Pin : 0~3]
  //  {S0,S1,S2,S3}
  {0,0,0,0}, //channel 0
  {1,0,0,0}, //channel 1
  {0,1,0,0}, //channel 2
  {1,1,0,0}, //channel 3   It means S0=1, S1=1, S2=0, S3=0
  {0,0,1,0}, //channel 4
  {1,0,1,0}, //channel 5
  {0,1,1,0}, //channel 6
  {1,1,1,0}, //channel 7
  {0,0,0,1}, //channel 8    It means S0=0, S1=0, S2=0, S3=1
  {1,0,0,1}, //channel 9
  {0,1,0,1}, //channel 10
  {1,1,0,1}, //channel 11
  {0,0,1,1}, //channel 12
  {1,0,1,1}, //channel 13
  {0,1,1,1}, //channel 14
  {1,1,1,1}  //channel 15
};

/*
  USAGE : int value = read_1ch_in_mux(i); // i = 0~15
*/
int read_1ch_in_mux(int mux_ch) {
  // config signal 4 pins 
  for(int i = 0; i < NUM_MUX_SIG ; i++){
    digitalWrite(pinMuxSig[i], muxSig4PinsVal[mux_ch][i]);
  }

  int adc_raw = analogRead(pinADC);

  return adc_raw;
}


/*
  USAGE : int value[16]; read_16ch_in_mux(value); // value buffer will be filled.
*/
void read_16ch_in_mux(int * buf_16) {
  for (int i = 0 ; i < NUM_MUX_OUT ; i++) {
      buf_16[i] = read_1ch_in_mux(i);
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
void read_16ch_in_mux_fast(int * buf_16) {
  //  ch 0, set all LOW
  for(int i = 0; i < NUM_MUX_SIG ; i++){
    digitalWrite(pinMuxSig[i], 0);
  }
  buf_16[0] = analogRead(pinADC);

  digitalWrite(S0, 1); //  ch 1 : 1000
  buf_16[1] = analogRead(pinADC);
  digitalWrite(S1, 1); //  ch 3 : 1100
  buf_16[3] = analogRead(pinADC);
  digitalWrite(S2, 1); //  ch 7 : 1110
  buf_16[7] = analogRead(pinADC);
  digitalWrite(S3, 1); //  ch 15 : 1111
  buf_16[15] = analogRead(pinADC);

  digitalWrite(S0, 0); //  ch 14 : 0111
  buf_16[14] = analogRead(pinADC);
  digitalWrite(S1, 0); //  ch 12 : 0011
  buf_16[12] = analogRead(pinADC);
  digitalWrite(S2, 0); //  ch 8 : 0001
  buf_16[8] = analogRead(pinADC);

  digitalWrite(S0, 1); //  ch 9 : 1001
  buf_16[9] = analogRead(pinADC);
  digitalWrite(S1, 1); //  ch 11 : 1101
  buf_16[11] = analogRead(pinADC);
  digitalWrite(S0, 0); //  ch 10 : 0101
  buf_16[10] = analogRead(pinADC);
  digitalWrite(S3, 0); //  ch 2 : 0100
  buf_16[2] = analogRead(pinADC);

  digitalWrite(S2, 1); //  ch 6 : 0110
  buf_16[6] = analogRead(pinADC);
  digitalWrite(S1, 0); //  ch 4 : 0010
  buf_16[4] = analogRead(pinADC);
  digitalWrite(S0, 1); //  ch 5 : 1010
  buf_16[5] = analogRead(pinADC);
  digitalWrite(S3, 1); //  ch 13 : 1011
  buf_16[13] = analogRead(pinADC);
}


//-----------------------------------------------------
//    SELECT MUX
//-----------------------------------------------------
void selectMux(int mux_id) {

  for(int i = 0 ; i < NUM_OF_MUX ; i++) {
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


