#pragma once

#include <stdint.h>

#include "app_state.h"

typedef struct {
	int high_side_gpio;
	int low_side_gpio;
	uint32_t carrier_frequency_hz;
	uint32_t timer_resolution_hz;
	uint32_t deadtime_ticks;
} pwm_hbridge_config_t;

void pwm_init(const pwm_hbridge_config_t *config);
void pwm_update(controller_t *controller);
