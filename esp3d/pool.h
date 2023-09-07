#include "Arduino.h"
class POOL
{
public:
    POOL();
    static void setIonPolarityCyclePeriod_ms(uint32_t cycle_period_ms = 2000);
    static void SwapPolarity();
    static void setup(uint32_t cycle_period_ms = 2000);
    static void setChlorineDuty(uint8_t duty = 128);
    static void setupClock();
private:

};
