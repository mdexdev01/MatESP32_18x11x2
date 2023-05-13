/*
  BlinkRGB

  Demonstrates usage of onboard RGB LED on some ESP dev boards.

  Calling digitalWrite(RGB_BUILTIN, HIGH) will use hidden RGB driver.

  RGBLedWrite demonstrates controll of each channel:
  void neopixelWrite(uint8_t pin, uint8_t red_val, uint8_t green_val, uint8_t blue_val)

  WARNING: After using digitalWrite to drive RGB LED it will be impossible to drive the same pin
    with normal HIGH/LOW level
*/
// #include <NeoPixel.h>

#define RGB_BRIGHTNESS 3 // Change white brightness (max 255)

// the setup function runs once when you press reset or power the board

void setup_RgbLed()
{
  // No need to initialize the RGB LED
}

// ex)  set_BuiltInRgbLed(3, 3, 0);
void set_BuiltInRgbLed(int r, int g, int b)
{
  neopixelWrite(RGB_BUILTIN, r, g, b);
}

//  ex) set_BuiltInRgbLedHex(0x050000);
void set_BuiltInRgbLedHex(int rgb)
{
  int r = (rgb & 0xFF0000) >> 16;
  int g = (rgb & 0x00FF00) >> 8;
  int b = rgb & 0x0000FF;
  neopixelWrite(RGB_BUILTIN, r, g, b);
}

void set_BuiltInRgbLedLoop(int index)
{
#define LOOP_LEN 80000
  int reminder = index % LOOP_LEN;

  int r = 0;
  int g = 0;
  int b = 0;

  if (reminder < LOOP_LEN / 4)
  {
    r = 10;
  }
  else if (reminder < LOOP_LEN * 2 / 4)
  {
    g = 10;
    b = 10;
  }
  else if (reminder < LOOP_LEN * 3 / 4)
  {
    r = 10;
    g = 10;
  }
  else
  {
    b = 10;
  }
  neopixelWrite(RGB_BUILTIN, r, g, b);
}

void loop_RgbLed_Ex1()
{
  set_BuiltInRgbLedHex(0x050000);
  delay(605);
  set_BuiltInRgbLed(3, 3, 0);
  delay(605);
}

// the loop function runs over and over again forever
void loop_RgbLed_Ex2()
{
#ifdef RGB_BUILTIN

  // digitalWrite(RGB_BUILTIN, HIGH);   // Turn the RGB LED white
  // delay(605);
  // digitalWrite(RGB_BUILTIN, LOW);    // Turn the RGB LED off
  // delay(605);

  neopixelWrite(RGB_BUILTIN, 0, 0, RGB_BRIGHTNESS); // Blue
  delay(605);
  neopixelWrite(RGB_BUILTIN, 0, RGB_BRIGHTNESS / 2, RGB_BRIGHTNESS); // GBB
  delay(605);
  neopixelWrite(RGB_BUILTIN, 0, RGB_BRIGHTNESS, RGB_BRIGHTNESS); // GGBB
  delay(605);
  neopixelWrite(RGB_BUILTIN, 0, RGB_BRIGHTNESS, RGB_BRIGHTNESS / 2); // GGB
  delay(605);
  neopixelWrite(RGB_BUILTIN, 0, RGB_BRIGHTNESS, 0); // Green
  delay(605);
  neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS / 2, RGB_BRIGHTNESS, 0); // RGG
  delay(605);
  neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS, RGB_BRIGHTNESS, 0); // RRGG
  delay(605);
  {
    neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS + 3, RGB_BRIGHTNESS + 3, 0); // RRGG
    delay(605);
    neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS + 5, RGB_BRIGHTNESS + 5, 0); // RRGG
    delay(605);
    neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS + 10, RGB_BRIGHTNESS + 10, 0); // RRGG
    delay(605);
  }
  neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS, RGB_BRIGHTNESS / 2, 0); // RRG
  delay(605);
  neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS, 0, 0); // Red
  delay(605);

  neopixelWrite(RGB_BUILTIN, 0, 0, 0); // Off / black
  delay(605);

#endif
}

void loop_RgbLed_Ex3()
{
#ifdef RGB_BUILTIN

  // digitalWrite(RGB_BUILTIN, HIGH);   // Turn the RGB LED white
  // delay(605);
  // digitalWrite(RGB_BUILTIN, LOW);    // Turn the RGB LED off
  // delay(605);

  neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS, 0, 0); // Red
  delay(605);
  neopixelWrite(RGB_BUILTIN, 0, 0, 0); // Off / black
  delay(605);
  neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS, 0, 0); // Red
  delay(605);
  neopixelWrite(RGB_BUILTIN, 0, 0, 0); // Off / black
  delay(605);
  neopixelWrite(RGB_BUILTIN, 0, RGB_BRIGHTNESS, 0); // Green
  delay(605);
  neopixelWrite(RGB_BUILTIN, 0, 0, 0); // Off / black
  delay(605);
  neopixelWrite(RGB_BUILTIN, 0, RGB_BRIGHTNESS, 0); // Green
  delay(605);
  neopixelWrite(RGB_BUILTIN, 0, 0, 0); // Off / black
  delay(605);
  neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS, RGB_BRIGHTNESS, 0); // Yellow
  delay(605);
  neopixelWrite(RGB_BUILTIN, 0, 0, 0); // Off / black
  delay(605);
  neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS, RGB_BRIGHTNESS, 0); // Yellow
  delay(605);
  neopixelWrite(RGB_BUILTIN, 0, 0, 0); // Off / black
  delay(605);

  neopixelWrite(RGB_BUILTIN, 0, 0, RGB_BRIGHTNESS); // Blue
  delay(605);
  neopixelWrite(RGB_BUILTIN, 0, 0, 0); // Off / black
  delay(605);
  neopixelWrite(RGB_BUILTIN, 0, 0, RGB_BRIGHTNESS); // Blue
  delay(605);
  neopixelWrite(RGB_BUILTIN, 0, 0, 0); // Off / black
  delay(605);

  neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS, RGB_BRIGHTNESS, RGB_BRIGHTNESS); // On / White
  delay(605);
  neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS, RGB_BRIGHTNESS, RGB_BRIGHTNESS); // On / White
  delay(605);

#endif
}
