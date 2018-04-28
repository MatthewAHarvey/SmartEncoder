#ifndef MILLITIMER_H
#define MILLITIMER_H

/*
 * This class sets up a tidy timer which allows the caller to check how much time has elapsed since it was last polled and report whether a minimum interval has been passed.
 * This allows events to be setup to occur at minimum set intervals within a block of code.
  */

class MilliTimer
{ 
    volatile unsigned long start;
    volatile unsigned long current;
    volatile unsigned long timeOut;
  public:
    MilliTimer();
    MilliTimer(unsigned long t);
    void init(unsigned long t); //initialisation function to be used if the timer is created with the null constructor.
    unsigned long elapsed();// return the elapsed time.
    //bool timedOut();// function to check whether the timer has timed out. If it has, reports true and resets the timer
    bool timedOut(bool RESET = false);
    void updateTimeOut(unsigned long t); //update the timeout variable without restarting the current time
    unsigned long getTimeOut();
    void reset(); // reset the timer.      
};

#endif 
