#include "Arduino.h"
#include "config.h"
#define POOL_SETTINGS_SIZE_BYTES 26
class POOL{
public:
    POOL();
    static void setup(uint32_t cycle_period_ms = 2000);
    static void loop();
    static bool setAutoMode(int mode);
    static bool pumpOnOff(bool on);
    static bool getPumpStatus();
    static bool chlorineOnOff(bool on);
    static bool getChlorineStatus();
    static bool ionOnOff(bool on);
    static bool getIonStatus();
    //chlorine duty
    static void setChlorineDuty(int dutyPercent = 50);
    static String getChlorineDuty();
    //ion duty
    static void setIonDuty(int dutyPercent = 50);
    static String getIonDuty();
    //current time
    static void setDateTime(const char *sDateTime);
    //start time
    static String getStartTime();
    static void setStartTime(const char *sTime);
    //stop time
    static String getStopTime();
    static void setStopTime(const char *sTime);
    //chlorine reverse cycle time
    static void setChlorineCyclePeriod(int period_ms = 30000);//30 sec default
    static String getChlorineCyclePeriod();
    //ion reverse cycle time
    static void setIonCyclePeriod(int period_ms = 30000);//30 sec default
    static String getIonCyclePeriod();
private:
    static void setOutputs();
    static void setChlorine();
    static void setIon();
    static void setupClock();
    static void calculateAlarmDelay();
    static void setAutoOnOffTimer();
    static void IRAM_ATTR alarm_autoOnOff();//called by timer interrupt to turn pool on and off;
    static void IRAM_ATTR alarm_swapChlorinePolarity();
    static void alarm_swapIonPolarity();
};
