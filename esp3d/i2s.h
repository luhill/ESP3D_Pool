#pragma once

#include <stdint.h>

// current value of the outputs provided over i2s
extern uint32_t i2s_port_data;

int i2s_init();

uint8_t i2s_state(uint8_t pin);

void i2s_write(uint8_t pin, uint8_t val);

void i2s_push_sample();
