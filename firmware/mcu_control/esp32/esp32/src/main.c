#include <stdbool.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "oled.h"
#include "pwm.h"
#include "fan_pwm.h"

#define PIN_FAN 27


static const pwm_hbridge_config_t s_pwm_config = {
    .high_side_gpio = 25,
    .low_side_gpio = 26,
    .carrier_frequency_hz = 20000U,
    .timer_resolution_hz = 1000000U,
    .deadtime_ticks = 2U,
};

static const char *TAG = "falownik";

static const float EXAMPLE_FREQUENCY_HZ = 50.0f;
static const float EXAMPLE_AC_VOLTAGE_V = 230.0f;
static const float EXAMPLE_AC_CURRENT_A = 2.5f;
static const float EXAMPLE_DC_VOLTAGE_V = 24.0f;
static const float EXAMPLE_DC_CURRENT_A = 10.0f;
static const float EXAMPLE_TEMP_T1_C = 36.0f;
static const float EXAMPLE_TEMP_T2_C = 31.0f;
static const float EXAMPLE_FAN_SPEED_PERCENT = 25.0f;

static controller_t s_controller = {
    .enabled = true,
    .frequency_hz = EXAMPLE_FREQUENCY_HZ,
    .amplitude_percent = 80.0f,
    .ac_voltage_v = EXAMPLE_AC_VOLTAGE_V,
    .ac_current_a = EXAMPLE_AC_CURRENT_A,
    .dc_voltage_v = EXAMPLE_DC_VOLTAGE_V,
    .dc_current_a = EXAMPLE_DC_CURRENT_A,
    .temp_t1_c = EXAMPLE_TEMP_T1_C,
    .temp_t2_c = EXAMPLE_TEMP_T2_C,
    .fan_speed_percent = EXAMPLE_FAN_SPEED_PERCENT,
    .phase = 0.0f,
    .last_pwm_update_us = 0,
    .last_ui_update_us = 0,
    .ui_dirty = true,
};

void app_main(void)
{
    ESP_LOGI(TAG, "Start");

    pwm_init(&s_pwm_config);
    oled_init();
    fan_init(PIN_FAN);

    /* I2C scanner removed; no scan performed here */

    oled_show_boot();
    oled_render_ui(&s_controller);
    s_controller.last_ui_update_us = esp_timer_get_time();

    while (true) {
        pwm_update(&s_controller);
        /* update fan PWM from controller state */
        fan_set_speed_percent(s_controller.fan_speed_percent);

        int64_t now_us = esp_timer_get_time();
        if (s_controller.ui_dirty || (now_us - s_controller.last_ui_update_us) > 200000) {
            oled_render_ui(&s_controller);
            s_controller.last_ui_update_us = now_us;
            s_controller.ui_dirty = false;
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
