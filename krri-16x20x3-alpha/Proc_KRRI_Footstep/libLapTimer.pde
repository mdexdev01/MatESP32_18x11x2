class LapTimer {
    ControlTimer timerMine;
    long        tickOrigin;
    long        lastCycle = 0;
    String      nickName;

    LapTimer(String nick) {
        timerMine = new ControlTimer();
        timerMine.setSpeedOfTime(1); // 1 : 1ms
        tickOrigin = timerMine.time(); //<>//
        lastCycle = 0;
        
        nickName = nick;

        println("timer starts... " + nickName);
   };

    String getNick() {
        return nickName;
    }

    void reset() {
        tickOrigin = timerMine.time(); //<>//
        lastCycle = 0;
    }

    long getElapsedTick() {
        return (timerMine.time() - tickOrigin); //<>//
    }

    void updateLastCycle(long last_cycle) {
        lastCycle = last_cycle;
    }

    long getLastCycle() {
        return lastCycle;
    }

};


String getCurTimeString() {
    int d = day();    // Values from 1 - 31
    int mo = month();  // Values from 1 - 12
    int y = year();   // 2003, 2004, 2005, etc.

    int s = second();  // Values from 0 - 59
    int m = minute();  // Values from 0 - 59
    int h = hour();    // Values from 0 - 23

    String time_string = String.format("%4d%02d%02d-%02d%02d%02d", y, mo, d, h, m, s);

    return time_string;  
}
