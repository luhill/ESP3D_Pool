#include "pool.h"
#include <driver/ledc.h>

#include <time.h>
#include <sys/time.h>

#define PIN_ION_STEP 2 //stepper driver step pin is used to reverse polarity;
#define PIN_ION_ENA 14
#define PIN_CHLORINE_DUTY 12
void IRAM_ATTR SwapPolarity();
hw_timer_t *Timer0_Cfg = NULL;

uint16_t prescale = 40;
uint32_t ticks_per_ms = 80000000/prescale/1000/2;//80Mhz/prescale/1000ms per second/ 2 (timer must fire twice every time period to trigger stepper driver )

POOL::POOL(){}

void POOL::setup(uint32_t cycle_period_ms){
    pinMode(PIN_CHLORINE_DUTY,OUTPUT);
    pinMode(PIN_ION_STEP,OUTPUT);
    Timer0_Cfg = timerBegin(0, 40, true);
    timerAttachInterrupt(Timer0_Cfg, &SwapPolarity, true);
    setIonPolarityCyclePeriod_ms(cycle_period_ms);
    timerAlarmEnable(Timer0_Cfg);
    ledcSetup(1, 200, 8);

  // attach the channel to the GPIO to be controlled
    ledcAttachPin(PIN_CHLORINE_DUTY, 1);
    setChlorineDuty(128);
    setupClock();
}
void POOL::setupClock(){
    struct tm tm;
    tm.tm_year = 2023 - 1900;
    tm.tm_mon = 8;
    tm.tm_mday = 6;
    tm.tm_hour = 16;
    tm.tm_min = 40;
    tm.tm_sec = 10;
    time_t t = mktime(&tm);
    struct timeval now = { .tv_sec = t };
    settimeofday(&now, NULL);
}
/*
The stepper driver direction pin does not reverse polarity. The polarity is reversed when the driver takes a step like so:
           Winding:  A  |  B
Initial posisition:  +  |  -
            step 2:  +  |  +
            step 3:  -  |  +
            step 4:  -  |  -

Stepping in one direction takes two steps to reverse the polarity. However if the direction is changed after each step,
a single step can cause the polarity to change. Connecting the stepper driver Direction pin to the output of one of the coils
ensures that the direction changes every step causing the polarity to change every step (takes a maximum of 3 steps to settle into the oscillating state)
This shortcut reduces the number of timer interrupts and pin state changes required during operation. Note that the stepper driver
requires the step pin to be toggled high and then back to low again (requires a minium delay time) for each step (two operations).
Firing this timer twice per cycle period causes the desired cycle time. Timer period is therefore set to half of the cycle period.
*/
void POOL::SwapPolarity(){
  digitalWrite(PIN_ION_STEP, !digitalRead(PIN_ION_STEP));//invert pin output
}

//Begin which setup everything
void POOL::setIonPolarityCyclePeriod_ms(uint32_t cycle_period_ms){
    timerAlarmWrite(Timer0_Cfg, ticks_per_ms*cycle_period_ms, true);
}
void POOL::setChlorineDuty(uint8_t duty){
    ledcWrite(1,duty);
}
