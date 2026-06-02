#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool enabled;
    float frequency_hz;
    float amplitude_percent;
    float ac_voltage_v;
    float ac_current_a;
    float dc_voltage_v;
    float dc_current_a;
    float temp_t1_c;
    float temp_t2_c;
    float fan_speed_percent;
    float phase;
    int64_t last_pwm_update_us;
    int64_t last_ui_update_us;
    bool ui_dirty;
} controller_t;
