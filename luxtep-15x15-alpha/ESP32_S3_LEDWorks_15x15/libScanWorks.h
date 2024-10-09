#include <mdex_util24.h>
          
const int NUM_OF_MUX = 15;
int pinMuxEn[NUM_OF_MUX]    = { 41, 7, 40, 15, 39, 16, 38, 17, 37, 18, 36, 8, 35, 19, 20}; // Luxtep 15x15

// const int NUM_OF_MUX = 2;
// int pinMuxEn[NUM_OF_MUX]    = {21, 14}; // Luxtep 15x15

#define MUX_LIST_LEN  NUM_OF_MUX
int active_mux_index = -1;


//  Num of Mux output
#define NUM_MUX_OUT 16

//  ADC Buffer
int adc_value[NUM_OF_MUX][NUM_MUX_OUT];
// int **adc_value;

//  HW Pin # = Signal 0~3 of all mux
const int NUM_MUX_SIG = 4;

int pinMuxSig[NUM_MUX_SIG] = {2, 5, 42, 6}; // Luxtep 15x15
// int pinMuxSig[NUM_MUX_SIG] = {10, 11, 12, 13}; // Luxtep 15x15
int S0, S1, S2, S3;

// int pinADC = A8;
int pinADC = A0;
#define RESOLUTION_BITS (8) // choose resolution (explained in depth below)


void adcScanMainPage();
void adcPlanePage();
void print_adc();

MDEX_Util24 * MdexUtil;

void setup_adc() {
  analogReadResolution(RESOLUTION_BITS); // set the resolution of analogRead results
                                          //  - maximum: 14 bits (maximum ADC resolution)
                                          //  - default: 10 bits (standard Arduino setting)
                                          //  - minimum:  1 bit
  pinMode(pinADC, INPUT);

  for (int i = 0; i < NUM_OF_MUX; i++){
      pinMode(pinMuxEn[i], OUTPUT);
      digitalWrite(pinMuxEn[i], HIGH);
  }

  for (int i = 0; i < NUM_MUX_SIG; i++){
      pinMode(pinMuxSig[i], OUTPUT);
  }
  S0 = pinMuxSig[0];
  S1 = pinMuxSig[1];
  S2 = pinMuxSig[2];
  S3 = pinMuxSig[3];

  MdexUtil = new MDEX_Util24();

  delay(5);
}

int muxSig4PinsVal[NUM_MUX_OUT][NUM_MUX_SIG] = {
  // [Mux Output : 0~15][Signal Pin : 0~3]
  //  {S0,S1,S2,S3}
  {0, 0, 0, 0}, // channel 0
  {1, 0, 0, 0}, // channel 1
  {0, 1, 0, 0}, // channel 2
  {1, 1, 0, 0}, // channel 3   It means S0=1, S1=1, S2=0, S3=0
  {0, 0, 1, 0}, // channel 4
  {1, 0, 1, 0}, // channel 5
  {0, 1, 1, 0}, // channel 6
  {1, 1, 1, 0}, // channel 7
  {0, 0, 0, 1}, // channel 8    It means S0=0, S1=0, S2=0, S3=1
  {1, 0, 0, 1}, // channel 9
  {0, 1, 0, 1}, // channel 10
  {1, 1, 0, 1}, // channel 11
  {0, 0, 1, 1}, // channel 12
  {1, 0, 1, 1}, // channel 13
  {0, 1, 1, 1}, // channel 14
  {1, 1, 1, 1}  // channel 15
};

/*
  USAGE : int value = read_1ch_in_mux(i); // i = 0~15
*/
int read_1ch_in_mux(int mux_ch, int cut_off){
    // config signal 4 pins
    for (int i = 0; i < NUM_MUX_SIG; i++){
        digitalWrite(pinMuxSig[i], muxSig4PinsVal[mux_ch][i]);
    }

    int adc_raw = analogRead(pinADC);

    adc_raw = MDEX_Util24::conv_1515a(adc_raw, cut_off, 240, 0);

    return adc_raw;
}

/*
  USAGE : int value[16]; read_16ch_in_mux(value); // value buffer will be filled.
*/
void read_16ch_in_mux(int *buf_16, int cut_off){
    for (int i = 0; i < NUM_MUX_OUT; i++){
        buf_16[i] = read_1ch_in_mux(i, cut_off);
        // delay(5);
    }
}

void read_16ch_in_mux_fast(int *buf_16)
{
    //  ch 0, set all LOW
    for (int i = 0; i < NUM_MUX_SIG; i++)
    {
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
void selectMux(int mux_id)
{
    for (int i = 0; i < NUM_OF_MUX; i++)
    {
        // for(int i = (NUM_OF_MUX - 1) ; 0 <= i ; i--) {
        digitalWrite(pinMuxEn[i], 1);
    }

    digitalWrite(pinMuxEn[mux_id], 0);

    active_mux_index = mux_id;
}

void changeMux(int mux_id)
{
    digitalWrite(pinMuxEn[active_mux_index], 1);

    digitalWrite(pinMuxEn[mux_id], 0);

    active_mux_index = mux_id;
}


bool adc_scan_done = false;

int loop_count_adc = 0;

void loop_adc(){
  // Serial.printf("\nLOOP Count2 = %d \n", loop_count_adc);
  if (adc_scan_done == false) {
      adcScanMainPage();
      // adcPlanePage();

      adc_scan_done = true;
  } else {

      //  single task mode
      {
          // buildPacket(packetBuf, adc_value, MUX_LIST_LEN, NUM_MUX_OUT);

          // sendPacket(packetBuf, PACKET_LEN);
      }

      delay(1);
  }

  // print_adc();
  // loop_wdTimer();

  loop_count_adc++;

}

int adc_scan_count_main = 0;
void adcScanMainPage() {
    // Serial.printf("[%d] ADC Full scan - main\n", adc_scan_count_main);
    //  CHANGE MUX AND READ ADC 16EA
    for (int mux_id = 0; mux_id < MUX_LIST_LEN; mux_id++) {
      //  Scan Option 1 : Manually
      selectMux(mux_id);
      read_16ch_in_mux(adc_value[mux_id], 1);  //  adc_value : 2d array, 10: threashold

      //  Scan option 2 : Minimal change
      // changeMux(mux_id);
      // read_16ch_in_mux_fast(adc_value[mux_id]);  //  adc_value : 2d array
    }

    // selectMux(2);
    // adc_value[2][2] = read_1ch_in_mux(2, 0);

    adc_scan_count_main++;
}

void adcPlanePage() {
  int sum_val = 0;
  int average = 0;
  for (int mux_id = 0; mux_id < MUX_LIST_LEN; mux_id++) {
    for (int i = 0; i < NUM_MUX_OUT; i++){
        sum_val += adc_value[mux_id][i];
    }
  }

  average = sum_val / 225;

  for (int mux_id = 0; mux_id < MUX_LIST_LEN; mux_id++) {
    for (int i = 0; i < NUM_MUX_OUT; i++){
        if( adc_value[mux_id][i] < (average + 3) ) {
          adc_value[mux_id][i] = 0;
        }
        else {
          adc_value[mux_id][i] -= average;
          adc_value[mux_id][i] *= 4;
        }
    }
  }

}


void print_adc() {
  // adc_value[mux_id]
  for (int mux_id = 0 ; mux_id < NUM_OF_MUX ; mux_id++) {
      Serial.printf("[mux:%2d] ", mux_id);

      for (int i = 0; i < NUM_MUX_OUT; i++) {
          Serial.printf("%3d,", adc_value[mux_id][i]);
      }
      Serial.println("~");
  }

}