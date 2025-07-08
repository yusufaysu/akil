#pragma once

#include <stdbool.h>
#include <stdint.h>

#define SDA  GPIO_NUM_5
#define SCL  GPIO_NUM_4

void ft6x36_init(void);
bool ft6x36_get_touch(uint16_t *x, uint16_t *y);
