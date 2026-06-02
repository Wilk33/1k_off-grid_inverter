#pragma once

#include <stdint.h>

/* Initialize fan PWM on given GPIO. Safe to call once during startup. */
void fan_init(int gpio);

/* Set fan speed in percent (0.0 - 100.0). Values outside clamped. */
void fan_set_speed_percent(float percent);

/* Stop fan PWM (sets duty to 0) */
void fan_stop(void);
