import controlP5.*; // import controlP5 library

ControlTimer lapTimer;
long         tickOrigin;
long last_grant_tick = 0;

void setup_Timer() {
  lapTimer = new ControlTimer();
  lapTimer.setSpeedOfTime(1); // 1 : 1ms
  
  reset_Timer();
}

void reset_Timer() {
  tickOrigin = lapTimer.time();
  last_grant_tick = 0;
}

long getElapsedTick_Timer() {
  return (lapTimer.time() - tickOrigin);
}

void setGrant_Timer(long grant_tick) { // updateGrant_Timer
  last_grant_tick = grant_tick;
}

void setGrantNow_Timer() { // updateGrant_Timer
  last_grant_tick = getElapsedTick_Timer();
}

long getGrantElapsed_Timer() {
  return (getElapsedTick_Timer() - last_grant_tick);
}

long getLastGrant_Timer() {
  return last_grant_tick;
}
