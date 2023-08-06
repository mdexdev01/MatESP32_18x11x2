#include "lib_rle.h"

//-------------------------------------
#define SRC_BUF_SIZE (500 + 5)
#define ENC_BUF_SIZE (500 + 5)
#define DEC_BUF_SIZE (500 + 5)
uint8_t ori_buffer[SRC_BUF_SIZE];
uint8_t enc_buffer[ENC_BUF_SIZE];
uint8_t dec_buffer[DEC_BUF_SIZE];

int enc_count = 0;

bool is_same();
void make_buffer();

void rle_test() {
    make_buffer();

    // rle_encode(uint8_t *in, int in_size, uint8_t *out, int out_size)
    int enc_size = rle_encode(ori_buffer, SRC_BUF_SIZE, enc_buffer, ENC_BUF_SIZE);

    Serial.printf("---- enc size = %d ---- \n", enc_size);
    Serial.println("ori buffer");
    for (int i = 0; i < SRC_BUF_SIZE; i++) {
        Serial.printf("%2d,", ori_buffer[i]);
        if (i % 10 == 9) {
            Serial.println("");
        }
    }

    Serial.println("enc buffer");
    for (int i = 0; i < ENC_BUF_SIZE; i++) {
        Serial.printf("0x%02x,", enc_buffer[i]);
        if (i % 10 == 9) {
            Serial.println("");
        }
    }

    // rle_decode(uint8_t *in, int in_size, uint8_t *out, int out_size)
    int dec_size = rle_decode(enc_buffer, enc_size, dec_buffer, DEC_BUF_SIZE);

    Serial.printf("---- dec size = %d ---- \n", dec_size);
    Serial.println("enc buffer");
    for (int i = 0; i < SRC_BUF_SIZE; i++) {
        Serial.printf("0x%02x,", enc_buffer[i]);
        if (i % 10 == 9) {
            Serial.println("");
        }
    }

    Serial.println("dec buffer");
    for (int i = 0; i < DEC_BUF_SIZE; i++) {
        Serial.printf("%2d,", dec_buffer[i]);
        if (i % 10 == 9) {
            Serial.println("");
        }
    }

    bool is_ok = is_same();
    if (is_ok == true) {
        Serial.println("OK");
    } else {
        Serial.println("FAIL");
    }

    delay(2000);

    enc_count++;
}


void make_buffer() {
    for (int i = 0; i < SRC_BUF_SIZE; i++) {
        if ((8 < i) && (i < 14)) {
            ori_buffer[i] = 0;
        } else if ((22 < i) && (i < 34)) {
            ori_buffer[i] = 0;
        } else {
            int value = i * 8 + 160;

            value = value % 240;

            ori_buffer[i] = value;
        }
    }
}

bool is_same() {
  for (int i = 0; i < SRC_BUF_SIZE; i++) {
      if (ori_buffer[i] != dec_buffer[i]) {
          return false;
      }
  }
  return true;
}


bool is_same_buf(byte * buf1, byte * buf2, int buf_size ) {
  if(50000 < buf_size) 
    return false;
  
  for (int i = 0; i < buf_size; i++) {
      if (buf1[i] != buf2[i]) {
          return false;
      }
  }

  return true;
}

/*
---- enc size = 43 ----
ori buffer
160,168,176,184,192,200,208,216,224, 0,
 0, 0, 0, 0,32,40,48,56,64,72,
80,88,96, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0,192,200,208,216,224,232,
 0, 8,16,24,32,40,48,56,64,72,
enc buffer
0x8a,0xa0,0xa8,0xb0,0xb8,0xc0,0xc8,0xd0,0xd8,0xe0,
0x00,0x02,0x00,0x8a,0x20,0x28,0x30,0x38,0x40,0x48,
0x50,0x58,0x60,0x00,0x08,0x00,0x90,0xc0,0xc8,0xd0,
0xd8,0xe0,0xe8,0x00,0x08,0x10,0x18,0x20,0x28,0x30,
0x38,0x40,0x48,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

---- dec size = 50 ----
enc buffer
0x8a,0xa0,0xa8,0xb0,0xb8,0xc0,0xc8,0xd0,0xd8,0xe0,
0x00,0x02,0x00,0x8a,0x20,0x28,0x30,0x38,0x40,0x48,
0x50,0x58,0x60,0x00,0x08,0x00,0x90,0xc0,0xc8,0xd0,
0xd8,0xe0,0xe8,0x00,0x08,0x10,0x18,0x20,0x28,0x30,
0x38,0x40,0x48,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
dec buffer
160,168,176,184,192,200,208,216,224, 0,
 0, 0, 0, 0,32,40,48,56,64,72,
80,88,96, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0,192,200,208,216,224,232,
 0, 8,16,24,32,40,48,56,64,72,
OK
*/