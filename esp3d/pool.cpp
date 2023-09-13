#include "pool.h"
#include <driver/ledc.h>
#include <EEPROM.h>
#include <time.h>
#include <sys/time.h>
#include "espcom.h"
#include "pins_tinybee.h"
#include "i2s.h"

#include "fastio.h"
/*-----MKS-TinyBee Pins---------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------------*/

#define PIN_PUMP_POWER          26
#define PIN_CHLORINE_DUTY_FWD   E0_DIR_PIN
#define PIN_CHLORINE_DUTY_REV   E0_STEP_PIN
#define PIN_ION_DUTY_FWD        E1_DIR_PIN
#define PIN_ION_DUTY_REV        E1_STEP_PIN

hw_timer_t *timer_autoOnOff = NULL;
hw_timer_t *timer_chlorinePolarity = NULL;
hw_timer_t *timer_ionPolarity = NULL;

uint16_t prescale_polarity = 40000;
uint16_t prescale_onOff = 64000;
uint32_t ticks_per_ms_polarity = 80000000/prescale_polarity/1000;//80Mhz/prescale/1000ms per second/ 2 (timer must fire twice every time period to trigger stepper driver )
uint32_t ticks_per_sec_onOff = 80000000/prescale_onOff;//80Mhz/prescales per min(timer must fire twice every time period to trigger stepper driver )

struct PoolParameters{//26 bytes
  byte initialized;//1 byte = flag, used to determine if the eeprom has been initialized with pool settings yet
  byte autoMode;//1 byte = flag
  int chlorine_duty;//4 bytes = int
  int ion_duty;//4 bytes = int
  int startTime;//4 bytes = int
  int stopTime;//4 bytes = int
  int chlorine_cycle_period;//4 bytes = int
  int ion_cycle_period;//4 bytes = int
};

#define chlorinePWMChannel_fwd 0
#define chlorinePWMChannel_rev 1
#define ionPWMChannel_fwd 2
#define ionPWMChannel_rev 3

static PoolParameters poolSettings;
static bool pumpOn = false;
static bool chlorineOn = true;
static bool chlorine_rev = false;
static bool ionOn = true;
static bool ion_rev = false;
static bool onActionScheduled = true;
static int onDelaySeconds;
static int offDelaySeconds;
static int nextAlarmSeconds;

POOL::POOL(){}

void POOL::setup(uint32_t cycle_period_ms){
    int i = i2s_init();

    //WRITE(Y_STEP_PIN,HIGH);
    /*pinMode(PIN_PUMP_POWER,OUTPUT);
    pinMode(PIN_CHLORINE_DUTY_FWD,OUTPUT);
    pinMode(PIN_CHLORINE_DUTY_REV,OUTPUT);
    pinMode(PIN_ION_DUTY_FWD,OUTPUT);
    pinMode(PIN_ION_DUTY_REV,OUTPUT);
  */
    setupClock();
    EEPROM.get(POOL_PARAMETERS_ADDRESS,poolSettings);
    if(!poolSettings.initialized){
      poolSettings.initialized = true;
      poolSettings.autoMode = true;
      poolSettings.chlorine_duty = 50;
      poolSettings.ion_duty = 50;
      poolSettings.startTime = 3600*8.5;//8:30 am
      poolSettings.stopTime = 3600*8.5+60;//60*15.5;//3:30 pm
      poolSettings.chlorine_cycle_period = 1000;//30sec
      poolSettings.ion_cycle_period = 1000;//15sec
      EEPROM.put(POOL_PARAMETERS_ADDRESS, poolSettings);
    }
    onDelaySeconds = 10;
    offDelaySeconds = 10;
    //chlorine reverse cycle timer
    timer_chlorinePolarity = timerBegin(0, prescale_polarity, true);
    timerAttachInterrupt(timer_chlorinePolarity, &alarm_swapChlorinePolarity, true);
    timerAlarmWrite(timer_chlorinePolarity, ticks_per_ms_polarity*poolSettings.chlorine_cycle_period, true);
    timerAlarmEnable(timer_chlorinePolarity);

    //ionizer reverse cycle timer
    timer_ionPolarity = timerBegin(1, prescale_polarity, true);
    timerAttachInterrupt(timer_ionPolarity, &alarm_swapIonPolarity, true);
    timerAlarmWrite(timer_ionPolarity, ticks_per_ms_polarity*poolSettings.ion_cycle_period, true);
    timerAlarmEnable(timer_ionPolarity);

    //auto on off timer
    timer_autoOnOff = timerBegin(2, prescale_onOff, true);
    timerAttachInterrupt(timer_autoOnOff, &alarm_autoOnOff, true);
    calculateAlarmDelay();
    setAutoOnOffTimer();
    timerAlarmEnable(timer_autoOnOff);

    //setup chlorine pwm
    ledcSetup(chlorinePWMChannel_fwd, 1000, 8);//1khz 8bits (0-255)
    ledcAttachPin(PIN_CHLORINE_DUTY_FWD, chlorinePWMChannel_fwd);
    ledcSetup(chlorinePWMChannel_rev, 1000, 8);//1khz 8bits (0-255)
    ledcAttachPin(PIN_CHLORINE_DUTY_REV, chlorinePWMChannel_rev);

    //setup ion pwm
    ledcSetup(ionPWMChannel_fwd, 1000, 8);//1khz 8bits (0-255)
    ledcAttachPin(PIN_ION_DUTY_FWD, ionPWMChannel_fwd);
    ledcSetup(ionPWMChannel_rev, 1000, 8);//1khz 8bits (0-255)
    ledcAttachPin(PIN_ION_DUTY_REV, ionPWMChannel_rev);

    setOutputs();
    SET_OUTPUT(X_ENABLE_PIN);
    WRITE(X_ENABLE_PIN,128);
    //i2s_push_sample();
    //pinMode(Y_STEP_PIN,OUTPUT);
    //i2s_write(0, HIGH);
}
void POOL::loop(){
}
void POOL::setAutoOnOffTimer(){
  if(poolSettings.autoMode){
    timerAlarmWrite(timer_autoOnOff, ticks_per_sec_onOff*nextAlarmSeconds, true);
    timerWrite(timer_autoOnOff,0);//ensures that if timer is changed while running it gets reset to 0;
  }else{
    if(timerStarted(timer_autoOnOff)){
      timerStop(timer_autoOnOff);
    }
  }
}
void POOL::alarm_autoOnOff(){
  pumpOn = onActionScheduled;
  chlorineOn = onActionScheduled;
  ionOn = onActionScheduled;
  onActionScheduled = !onActionScheduled;
  if(onActionScheduled){//calculate time until turn on alarm
    nextAlarmSeconds = onDelaySeconds;
  }else{//calculate time until turn off alarm
    nextAlarmSeconds = offDelaySeconds;
  }
  setOutputs();
  setAutoOnOffTimer();//resquedule the timer
}
void POOL::setupClock(){
    configTime(10*60*60, 0, "pool.ntp.org");//brisbane time 10hrs ahead , 0 daylight saving

    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){//error getting time from ntp server. set it manually
      timeinfo.tm_year = 2023 - 1900;
      timeinfo.tm_mon = 8;
      timeinfo.tm_mday = 6;
      timeinfo.tm_hour = 8;
      timeinfo.tm_min = 29;
      timeinfo.tm_sec = 30;
      time_t t = mktime(&timeinfo);
      struct timeval now = { .tv_sec = t };
      settimeofday(&now, NULL);
    }
    return;
}
void POOL::calculateAlarmDelay(){
    time_t now;
    time(&now);
    struct tm lTime;
    localtime_r(&now,&lTime);
    int secPerDay = 24*3600;
    int sec = lTime.tm_hour*3600 + lTime.tm_min*60 + lTime.tm_sec;
    if((sec < poolSettings.stopTime)*(sec > poolSettings.startTime)||(sec > poolSettings.stopTime)*(sec < poolSettings.startTime)){
      onActionScheduled = poolSettings.stopTime < poolSettings.startTime;
    }else{
      onActionScheduled = !(poolSettings.stopTime < poolSettings.startTime);
    }
    nextAlarmSeconds = min((poolSettings.startTime-sec+secPerDay)%secPerDay,(poolSettings.stopTime-sec+secPerDay)%secPerDay);
    onDelaySeconds = (poolSettings.stopTime - poolSettings.startTime + secPerDay)%secPerDay;
    offDelaySeconds = (poolSettings.stopTime - poolSettings.startTime + secPerDay)%secPerDay;
}
bool POOL::setAutoMode(int mode){
  if(mode < 2){
    if(mode != poolSettings.autoMode){
      poolSettings.autoMode = mode>0;
      setAutoOnOffTimer();
    }
  }
  return poolSettings.autoMode;
}
bool POOL::pumpOnOff(bool on){
  pumpOn = on;
  if(!pumpOn){//pump must be on for chlorinator and ionizer to run
    chlorineOn = false;
    ionOn = false;
  }
  setOutputs();
  return pumpOn;
}
bool POOL::getPumpStatus(){
  return pumpOn;
}
bool POOL::chlorineOnOff(bool on){
  chlorineOn = on && pumpOn;
  setChlorine();
  return chlorineOn;
}
bool POOL::getChlorineStatus(){
  return chlorineOn;
}
bool POOL::ionOnOff(bool on){
  ionOn = on && pumpOn;
  setIon();
  return ionOn;
}
bool POOL::getIonStatus(){
  return ionOn;
}
void POOL::setOutputs(){
  //digitalWrite(PIN_PUMP_POWER, pumpOn);
  //setChlorine();
  //setIon();
}
void POOL::setChlorine(){
  ledcWrite(chlorinePWMChannel_fwd,poolSettings.chlorine_duty*255/100*chlorineOn*!chlorine_rev);
  ledcWrite(chlorinePWMChannel_rev,poolSettings.chlorine_duty*255/100*chlorineOn*chlorine_rev);
  if(chlorineOn){
    if(!timerStarted(timer_chlorinePolarity)){timerStart(timer_chlorinePolarity);}
  }else{
    if(timerStarted(timer_chlorinePolarity)){timerStop(timer_chlorinePolarity);}
  }
}
void POOL::setIon(){
  ledcWrite(ionPWMChannel_fwd,poolSettings.ion_duty*255/100*ionOn*!ion_rev);
  ledcWrite(ionPWMChannel_rev,poolSettings.ion_duty*255/100*ionOn*ion_rev);
  if(ionOn){
    if(!timerStarted(timer_ionPolarity)){timerStart(timer_ionPolarity);}
  }else{
    if(timerStarted(timer_ionPolarity)){timerStop(timer_ionPolarity);}
  }
}
void POOL::setChlorineDuty(int duty_percent){
    if(duty_percent!=poolSettings.chlorine_duty){
      poolSettings.chlorine_duty = duty_percent;
      //todo save to eeprom
    }
    setChlorine();
}
String POOL::getChlorineDuty(){
  return String(poolSettings.chlorine_duty);
}
void POOL::setIonDuty(int duty_percent){
    if(duty_percent!=poolSettings.ion_duty){
      poolSettings.ion_duty = duty_percent;
      //todo save to eeprom
    }
}
String POOL::getIonDuty(){
  return String(poolSettings.ion_duty);
}
String POOL::getStartTime(){
  int h = floor(poolSettings.startTime/3600);
  int m = floor((poolSettings.startTime - h*3600)/60);
  char timeBuff[5];
  sprintf(timeBuff, "%02d:%02d", h, m);
  return timeBuff;
}
void POOL::setStartTime(const char *sTime){
  int tdata[2];
  sscanf(sTime,"%d:%d",&tdata[0],&tdata[1]);
  poolSettings.startTime = tdata[0]*3600+tdata[1]*60;
  calculateAlarmDelay();
  setAutoOnOffTimer();
}
String POOL::getStopTime(){
  int h = floor(poolSettings.stopTime/3600);
  int m = floor((poolSettings.stopTime - h*3600)/60);
  char timeBuff[5];
  sprintf(timeBuff, "%02d:%02d", h, m);
  return timeBuff;
}
void POOL::setStopTime(const char *sTime){
  int tdata[2];
  sscanf(sTime,"%d:%d",&tdata[0],&tdata[1]);
  poolSettings.stopTime = tdata[0]*3600+tdata[1]*60;
  calculateAlarmDelay();
  setAutoOnOffTimer();
}
void POOL::setDateTime(const char *sDateTime){//2023-09-11T16:30:21
  struct tm ti;
  sscanf(sDateTime,"%d-%d-%dT%d:%d",&ti.tm_year,&ti.tm_mon,&ti.tm_mday,&ti.tm_hour,&ti.tm_min);
  ti.tm_year-=1900;
  ti.tm_mon-=1;//0-11
  time_t t = mktime(&ti);
  struct timeval now = { .tv_sec = t };
  settimeofday(&now, NULL);
  calculateAlarmDelay();
  setAutoOnOffTimer();
}
void POOL::setChlorineCyclePeriod(int period_ms){
    //if(period_ms!=poolSettings.chlorine_cycle_period){
      poolSettings.chlorine_cycle_period = period_ms;
      timerAlarmWrite(timer_chlorinePolarity, ticks_per_ms_polarity*period_ms, true);
      //todo save to eeprom
    //}
}
String POOL::getChlorineCyclePeriod(){
  return String(poolSettings.chlorine_cycle_period);
}
void POOL::setIonCyclePeriod(int period_ms){
    //if(period_ms!=poolSettings.ion_cycle_period){
      poolSettings.ion_cycle_period = period_ms;
      timerAlarmWrite(timer_ionPolarity, ticks_per_ms_polarity*period_ms, true);
      //todo save to eeprom
    //}
}
String POOL::getIonCyclePeriod(){
  return String(poolSettings.ion_cycle_period);
}
void POOL::alarm_swapChlorinePolarity(){
    chlorine_rev = !chlorine_rev;
    setChlorine();
}
void POOL::alarm_swapIonPolarity(){
    ion_rev = !ion_rev;
    setIon();
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

void POOL::SwapChlorinePolarity(){
  digitalWrite(PIN_ION_STEP, !digitalRead(PIN_ION_STEP));//invert pin output
}
*/
//Begin which setup everything
