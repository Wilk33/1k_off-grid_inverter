#include <math.h>
#include <stdbool.h>

#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_timer.h"

#include "pwm.h"

#define PWM_MIN_DUTY 0.05f
#define PWM_MAX_DUTY 0.95f
#define PWM_TWO_PI 6.28318530718f

// Choose LEDC resolution (bits)
#define LEDC_DUTY_RES_BITS 10
#define LEDC_MAX_DUTY ((1 << LEDC_DUTY_RES_BITS) - 1)

static bool s_outputs_enabled = false;
static pwm_hbridge_config_t s_config;

static float pwm_clampf(float value, float min_value, float max_value)
{
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

static float pwm_wrap_phase(float phase)
{
    while (phase >= PWM_TWO_PI) phase -= PWM_TWO_PI;
    while (phase < 0.0f) phase += PWM_TWO_PI;
    return phase;
}

static float pwm_compute_delta_seconds(controller_t *controller)
{
    int64_t now_us = esp_timer_get_time();
    if (controller->last_pwm_update_us == 0) {
        controller->last_pwm_update_us = now_us;
        return 0.0f;
    }
    float delta_seconds = (float)(now_us - controller->last_pwm_update_us) / 1000000.0f;
    controller->last_pwm_update_us = now_us;
    return delta_seconds;
}

static float pwm_compute_normalized_duty(controller_t *controller, float delta_seconds)
{
    if (!controller->enabled) {
        controller->phase = 0.0f;
        return 0.0f;
    }

    float amplitude = pwm_clampf(controller->amplitude_percent, 0.0f, 100.0f) / 100.0f;
    controller->phase = pwm_wrap_phase(controller->phase + PWM_TWO_PI * controller->frequency_hz * delta_seconds);
    float normalized = 0.5f + 0.45f * amplitude * sinf(controller->phase);
    return pwm_clampf(normalized, PWM_MIN_DUTY, PWM_MAX_DUTY);
}

static void pwm_set_ledc_duty(int channel, uint32_t duty)
{
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, (ledc_channel_t)channel, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, (ledc_channel_t)channel));
}

void pwm_init(const pwm_hbridge_config_t *config)
{
    // copy config
    s_config = *config;

    // basic validation
    if (s_config.carrier_frequency_hz == 0) {
        // avoid division by zero / invalid timer
        s_config.carrier_frequency_hz = 10000;
    }

    // configure LEDC timer
    ledc_timer_config_t timer_cfg = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = LEDC_DUTY_RES_BITS,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = (int)s_config.carrier_frequency_hz,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    // configure two channels: high side -> channel 0, low side -> channel 1
    ledc_channel_config_t ch_high = {
        .gpio_num = s_config.high_side_gpio,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };

    ledc_channel_config_t ch_low = {
        .gpio_num = s_config.low_side_gpio,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };

    ESP_ERROR_CHECK(ledc_channel_config(&ch_high));
    ESP_ERROR_CHECK(ledc_channel_config(&ch_low));

    // start with outputs forced low
    pwm_set_ledc_duty(LEDC_CHANNEL_0, 0);
    pwm_set_ledc_duty(LEDC_CHANNEL_1, 0);
    s_outputs_enabled = false;
}

void pwm_update(controller_t *controller)
{
    float delta_seconds = pwm_compute_delta_seconds(controller);
    float normalized = pwm_compute_normalized_duty(controller, delta_seconds);
    // period in microseconds
    float period_us = 1000000.0f / (float)s_config.carrier_frequency_hz;
    // interpret s_config.deadtime_ticks as deadtime in microseconds for this LEDC implementation
    float deadtime_us = (float)s_config.deadtime_ticks;

    // deadtime fraction of period (clamp small/max to avoid disabling PWM)
    float dead_frac = 0.0f;
    if (period_us > 0.0f) {
        dead_frac = pwm_clampf(deadtime_us / period_us, 0.0f, 0.45f);
    }

    // create complementary outputs with software deadtime by shrinking both pulses by half the deadtime
    float high_adj = pwm_clampf(normalized - dead_frac * 0.5f, 0.0f, 1.0f);
    float low_adj = pwm_clampf((1.0f - normalized) - dead_frac * 0.5f, 0.0f, 1.0f);

    uint32_t duty_high = (uint32_t)(high_adj * (float)LEDC_MAX_DUTY);
    uint32_t duty_low = (uint32_t)(low_adj * (float)LEDC_MAX_DUTY);

    if (controller->enabled) {
        if (!s_outputs_enabled) s_outputs_enabled = true;
        // drive complementary channels with software deadtime
        pwm_set_ledc_duty(LEDC_CHANNEL_0, duty_high);
        pwm_set_ledc_duty(LEDC_CHANNEL_1, duty_low);
    } else {
        pwm_set_ledc_duty(LEDC_CHANNEL_0, 0);
        pwm_set_ledc_duty(LEDC_CHANNEL_1, 0);
        s_outputs_enabled = false;
        controller->phase = 0.0f;
    }
}
