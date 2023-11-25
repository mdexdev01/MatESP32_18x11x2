/**
* @class    LapTimer
* @brief    millisecond or interval timer
* @details  get the updated millisecond elapsed time. 
            And also provides the cycles. (simple get/set function)
*/
class LapTimer {
    ControlTimer timerMine;

    long        timeOrigin_ms = 0;
    String      nickName = "";

    LapTimer(String nick) {
        timerMine = new ControlTimer();
        timerMine.setSpeedOfTime(1); // 1 : 1ms
        timeOrigin_ms = timerMine.time(); //<>//
        
        nickName = nick;

        println("timer starts... [" + nickName + "]");
   };

    String getNick() {
        return nickName;
    }

    void reset() {
        timeOrigin_ms = timerMine.time(); //<>//
    }

    long getElapsed_ms() {
        return (timerMine.time() - timeOrigin_ms); //<>//
    }

    //-----------------------------------------------------
    // FUNCTIONS FOR CYCLE
    //-----------------------------------------------------
    long  cycleCheckedOutLast = 0;
    long cycleInterval_ms = 1000; // 1000 : 1 second
    long cycleTimerOrigin = 0;

    void cycleStart(long interval_ms) {
        cycleInterval_ms = interval_ms;
        cycleCheckedOutLast = 0;
        cycleTimerOrigin = timerMine.time(); //<>//
    }

    /**
     *  @brief  Caution: It cannot check the multiple interval. 
     *          it only check if the time past over the single interval. 
     */
    boolean cycleTryCheckOut() {
        long elapsed_ms = getElapsed_ms();
        long cycle_from_start = elapsed_ms / cycleInterval_ms;

        if(cycleCheckedOutLast < cycle_from_start) {
            cycleCheckedOutLast = cycle_from_start;
            return true;
        }
        else 
            return false;
    }

    long getLastCycle2() {
        return cycleCheckedOutLast;
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
