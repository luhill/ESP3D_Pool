#define SERVO0_PIN                            2

//
// Limit Switches
//
#define X_STOP_PIN                            33
#define Y_STOP_PIN                            32
#define Z_STOP_PIN                            22
//#define FIL_RUNOUT_PIN                        35

//
// Enable I2S stepper stream
//
#undef I2S_STEPPER_STREAM
#define I2S_STEPPER_STREAM
#define I2S_WS                                26
#define I2S_BCK                               25
#define I2S_DATA                              27
#undef LIN_ADVANCE                                // Currently, I2S stream does not work with linear advance

//
// Steppers
//
#define X_STEP_PIN                           129
#define X_DIR_PIN                            130
#define X_ENABLE_PIN                         128

#define Y_STEP_PIN                           132
#define Y_DIR_PIN                            133
#define Y_ENABLE_PIN                         131

#define Z_STEP_PIN                           135
#define Z_DIR_PIN                            136
#define Z_ENABLE_PIN                         134

#define E0_STEP_PIN                          138
#define E0_DIR_PIN                           139
#define E0_ENABLE_PIN                        137

#define E1_STEP_PIN                          141
#define E1_DIR_PIN                           142
#define E1_ENABLE_PIN                        140

#define Z2_STEP_PIN                          141
#define Z2_DIR_PIN                           142
#define Z2_ENABLE_PIN                        140

//
// Temperature Sensors
//
#define TEMP_0_PIN                            36  // Analog Input
#define TEMP_1_PIN                            34  // Analog Input, you need set R6=0Ω and R7=NC
#define TEMP_BED_PIN                          39  // Analog Input

//
// Heaters / Fans
//
#define HEATER_0_PIN                         145
#ifndef MKS_TEST
#define HEATER_1_PIN                         146
#define FAN_PIN                              147
#define FAN1_PIN                             148
#endif
#define HEATER_BED_PIN                       144

//#define CONTROLLER_FAN_PIN                 148
//#define E0_AUTO_FAN_PIN                    148  // need to update Configuration_adv.h @section extruder
//#define E1_AUTO_FAN_PIN                    149  // need to update Configuration_adv.h @section extruder
#ifdef MKS_TEST
#define HEATER_1_PIN_T                         146
#define FAN_PIN_T                              147
#define FAN1_PIN_T                             148
#define HEATER_BED_PIN_T                       144
#endif


//
// MicroSD card
//
#define SD_MOSI_PIN                           23
#define SD_MISO_PIN                           19
#define SD_SCK_PIN                            18
#define SDSS                                   5
#define SD_DETECT_PIN                         34 // IO34 default is SD_DET signal(Jump to SDDET)
#define USES_SHARED_SPI                       // SPI is shared by SD card with TMC SPI drivers

/**
 *                _____                                             _____
 * (BEEPER)IO149 | · · | IO13(BTN_ENC)             (SPI MISO) IO19 | · · | IO18 (SPI SCK)
 *  (LCD_EN)IO21 | · · | IO4(LCD_RS)                (BTN_EN1) IO14 | · · | IO5 (SPI CS)
 *  (LCD_D4)IO0  | · ·   IO16(LCD_D5)               (BTN_EN2) IO12 | · ·   23 (SPI MOSI)
 *  (LCD_D6)IO15 | · · | IO17(LCD_D7)               (SPI_DET) IO34 | · · | RESET
 *           GND | · · | 5V                                    GND | · · | 3.3V
 *                ￣￣￣                                            ￣￣￣
 *                EXP1                                               EXP2
 */

#define EXP1_03_PIN                         17
#define EXP1_04_PIN                         15
#define EXP1_05_PIN                         16
#define EXP1_06_PIN                         0
#define EXP1_07_PIN                         4
#define EXP1_08_PIN                         21
#define EXP1_09_PIN                         13
#define EXP1_10_PIN                         149

#define EXP2_03_PIN                         -1    // RESET
#define EXP2_04_PIN                         34
#define EXP2_05_PIN                         23
#define EXP2_06_PIN                         12
#define EXP2_07_PIN                         5
#define EXP2_08_PIN                         14
#define EXP2_09_PIN                         18
#define EXP2_10_PIN                         19
