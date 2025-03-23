
///////////////////////////////////////////////////////////////////////////////////////////////////
//      Examples
///////////////////////////////////////////////////////////////////////////////////////////////////

// Some functions of our own for creating animated effects -----------------

/*
  if you are using the Adafruit_NeoPixel library
  it has problems with ESP32 Arduino Core Version 3
  when there are more than 85 LEDs - I use ESP32 core V 2.0.17 for such projects
*/

// Fill strip pixels one after another with a color. Strip is NOT cleared
// first; anything there will be covered pixel by pixel. Pass in color
// (as a single 'packed' 32-bit value, which you can get by calling
// strip[i].Color(red, green, blue) as shown in the loop() function above),
// and a delay time (in milliseconds) between pixels.
void colorWipe(uint32_t color, int wait) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < strip[i].PixelCount(); j++) {  // For each pixel in strip[i]...
            strip[i].SetPixelColor(j, color);             //  Set pixel's color (in RAM)
            strip[i].Show();                              //  Update strip to match
            delay(wait);                                  //  Pause for a moment
        }
    }
}

// Theater-marquee-style chasing lights. Pass in a color (32-bit value,
// a la strip[i].Color(r,g,b) as mentioned above), and a delay time (in ms)
// between frames.
void theaterChase(uint32_t color, int wait) {
    for (int i = 0; i < 4; i++) {
        for (int a = 0; a < 10; a++) {     // Repeat 10 times...
            for (int b = 0; b < 3; b++) {  //  'b' counts from 0 to 2...
                strip[i].ClearTo(RgbColor(0, 0, 0));          //   Set all pixels in RAM to 0 (off)
                // 'c' counts up from 'b' to end of strip in steps of 3...
                for (int c = b; c < strip[i].PixelCount(); c += 3) {
                    strip[i].SetPixelColor(c, color);  // Set pixel 'c' to value 'color'
                }
                strip[i].Show();  // Update strip with new contents
                delay(wait);      // Pause for a moment
            }
        }
    }
}

// Rainbow cycle along whole strip[i]. Pass delay time (in ms) between frames.
void rainbow(int wait_us) {
    // Hue of first pixel runs 5 complete loops through the color wheel.
    // Color wheel has a range of 65536 but it's OK if we roll over, so
    // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
    // means we'll make 5*65536/256 = 1280 passes through this loop:
    for (long firstPixelHue = 0; firstPixelHue < 1 * 65536; firstPixelHue += 256) {
        // strip[i].rainbow() can take a single argument (first pixel hue) or
        // optionally a few extras: number of rainbow repetitions (default 1),
        // saturation and value (brightness) (both 0-255, similar to the
        // ColorHSV() function, default 255), and a true/false flag for whether
        // to apply gamma correction to provide 'truer' colors (default true).
        for (int i = 0; i < 4; i++) {
            // strip[i].rainbow(firstPixelHue);
            // Above line is equivalent to:
            // strip[i].rainbow(firstPixelHue, 1, 255, 255, true);
            strip[i].Show();  // Update strip with new contents
        }

        if (1000 < firstPixelHue)
            return;

        delayMicroseconds(wait_us);  // Pause for a moment
    }
}

int reminder = 1;
// Rainbow-enhanced theater marquee. Pass delay time (in ms) between frames.
void theaterChaseRainbow(int wait) {
    for (int i = 0; i < 4; i++) {
        int firstPixelHue = 0;         // First pixel starts at red (hue 0)
                                       // for(int a=0; a<30; a++) {  // Repeat 30 times...
        for (int b = 0; b < 3; b++) {  //  'b' counts from 0 to 2...
            strip[i].ClearTo(RgbColor(0, 0, 0));          //   Set all pixels in RAM to 0 (off)
            // 'c' counts up from 'b' to end of strip in increments of 3...
            for (int c = b; c < strip[i].PixelCount(); c += 3) {
                // hue of pixel 'c' is offset by an amount to make one full
                // revolution of the color wheel (range 65536) along the length
                // of the strip (strip[i].numPixels() steps):
                int hue = firstPixelHue + c * 65536L / strip[i].PixelCount() + reminder;
                uint32_t color = hue;//strip[i].gamma32(strip[i].ColorHSV(hue));  // hue -> RGB
                strip[i].SetPixelColor(c, color);                           // Set pixel 'c' to value 'color'
            }
            strip[i].Show();              // Update strip with new contents
            delay(wait);                  // Pause for a moment
            firstPixelHue += 65536 / 90;  // One cycle of color wheel over 90 frames
        }
        // }
    }

    reminder += 3;
    if (30 < reminder)
        reminder = 10;
}
