#ifndef _STRIPEXAMPLE_H_
#define _STRIPEXAMPLE_H_

///////////////////////////////////////////////////////////////////////////////////////////////////
//      Examples
///////////////////////////////////////////////////////////////////////////////////////////////////

void colorWipe(unsigned int color, int wait);

void theaterChase(unsigned int color, int wait);

// Rainbow cycle along whole strip[i]. Pass delay time (in ms) between frames.
void rainbow(int wait_us);

// Rainbow-enhanced theater marquee. Pass delay time (in ms) between frames.
void theaterChaseRainbow(int wait);
#endif  // _STRIPEXAMPLE_H_