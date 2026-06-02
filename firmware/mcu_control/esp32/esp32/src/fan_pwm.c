#include "fan_pwm.h"

#include "esp_err.h"
#include "esp_log.h"
#include "driver/ledc.h"

static const char *TAG = "fan_pwm";

/* Using LEDC timer 0, channel 0 (high speed) to avoid touching MCPWM used for H-bridge */
#define FAN_LEDC_TIMER       LEDC_TIMER_0
#define FAN_LEDC_MODE        LEDC_HIGH_SPEED_MODE
#define FAN_LEDC_CHANNEL     LEDC_CHANNEL_0
#define FAN_LEDC_DUTY_RES    LEDC_TIMER_8_BIT
#define FAN_LEDC_FREQUENCY   25000 /* 25 kHz */

static int s_fan_gpio = -1;

void fan_init(int gpio)
{
    s_fan_gpio = gpio;

    ledc_timer_config_t timer_cfg = {
        .speed_mode = FAN_LEDC_MODE,
        .timer_num = FAN_LEDC_TIMER,
        .duty_resolution = FAN_LEDC_DUTY_RES,
        .freq_hz = FAN_LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    esp_err_t err = ledc_timer_config(&timer_cfg);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "ledc_timer_config failed: %d", err);
    }

    ledc_channel_config_t ch_cfg = {
        .gpio_num = s_fan_gpio,
        .speed_mode = FAN_LEDC_MODE,
        .channel = FAN_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = FAN_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    err = ledc_channel_config(&ch_cfg);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "ledc_channel_config failed: %d", err);
    }

    ESP_LOGI(TAG, "Fan PWM initialized on GPIO %d", s_fan_gpio);
}

void fan_set_speed_percent(float percent)
{
    if (s_fan_gpio < 0) return;
    if (percent < 0.0f) percent = 0.0f;
    if (percent > 100.0f) percent = 100.0f;

    uint32_t max_duty = (1 << LEDC_TIMER_8_BIT) - 1;
    uint32_t duty = (uint32_t)((percent / 100.0f) * (float)max_duty + 0.5f);

    esp_err_t err = ledc_set_duty(FAN_LEDC_MODE, FAN_LEDC_CHANNEL, duty);
    if (err == ESP_OK) {
        ledc_update_duty(FAN_LEDC_MODE, FAN_LEDC_CHANNEL);
    } else {
        ESP_LOGW(TAG, "ledc_set_duty failed: %d", err);
    }
}

void fan_stop(void)
{
    fan_set_speed_percent(0.0f);
}
