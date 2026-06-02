#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "oled.h"


#define OLED_I2C_PORT I2C_NUM_0
#define OLED_ADDR 0x3F
#define OLED_WIDTH 128
#ifndef OLED_HEIGHT
#define OLED_HEIGHT 64
#endif
#define OLED_PAGES (OLED_HEIGHT / 8)
#define PIN_I2C_SDA 21
#define PIN_I2C_SCL 22
#define OLED_NOMINAL_VOLTAGE_V 230.0f

static const char *TAG = "falownik";

void i2c_scan_and_log(i2c_port_t port)
{
    ESP_LOGI(TAG, "Starting I2C scan on port %d", port);
    const uint8_t probe_byte = 0x00;
    for (uint8_t addr = 1; addr < 127; ++addr) {
        esp_err_t err = i2c_master_write_to_device(port, addr, &probe_byte, 1, pdMS_TO_TICKS(50));
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Found I2C device at 0x%02X", addr);
        }
    }
    ESP_LOGI(TAG, "I2C scan complete");
}

static uint8_t s_oled_buf[OLED_WIDTH * OLED_PAGES];
static uint8_t s_oled_addr = OLED_ADDR;

static const uint8_t *glyph_for(char c)
{
    static const uint8_t blank[7] = {0, 0, 0, 0, 0, 0, 0};
    static const uint8_t digit_0[7] = {14, 17, 17, 17, 17, 17, 14};
    static const uint8_t digit_1[7] = {4, 12, 4, 4, 4, 4, 14};
    static const uint8_t digit_2[7] = {14, 17, 1, 2, 4, 8, 31};
    static const uint8_t digit_3[7] = {14, 17, 1, 6, 1, 17, 14};
    static const uint8_t digit_4[7] = {2, 6, 10, 18, 31, 2, 2};
    static const uint8_t digit_5[7] = {31, 16, 30, 1, 1, 17, 14};
    static const uint8_t digit_6[7] = {6, 8, 16, 30, 17, 17, 14};
    static const uint8_t digit_7[7] = {31, 1, 2, 4, 8, 8, 8};
    static const uint8_t digit_8[7] = {14, 17, 17, 14, 17, 17, 14};
    static const uint8_t digit_9[7] = {14, 17, 17, 15, 1, 2, 28};
    static const uint8_t letter_A[7] = {14, 17, 17, 31, 17, 17, 17};
    static const uint8_t letter_B[7] = {30, 17, 17, 30, 17, 17, 30};
    static const uint8_t letter_C[7] = {14, 17, 16, 16, 16, 17, 14};
    static const uint8_t letter_D[7] = {30, 17, 17, 17, 17, 17, 30};
    static const uint8_t letter_E[7] = {31, 16, 16, 30, 16, 16, 31};
    static const uint8_t letter_F[7] = {31, 16, 16, 30, 16, 16, 16};
    static const uint8_t letter_G[7] = {14, 17, 16, 23, 17, 17, 14};
    static const uint8_t letter_H[7] = {17, 17, 17, 31, 17, 17, 17};
    static const uint8_t letter_I[7] = {31, 4, 4, 4, 4, 4, 31};
    static const uint8_t letter_J[7] = {1, 1, 1, 1, 1, 17, 14};
    static const uint8_t letter_K[7] = {17, 18, 20, 24, 20, 18, 17};
    static const uint8_t letter_L[7] = {16, 16, 16, 16, 16, 16, 31};
    static const uint8_t letter_M[7] = {17, 27, 21, 21, 17, 17, 17};
    static const uint8_t letter_N[7] = {17, 25, 21, 19, 17, 17, 17};
    static const uint8_t letter_O[7] = {14, 17, 17, 17, 17, 17, 14};
    static const uint8_t letter_P[7] = {30, 17, 17, 30, 16, 16, 16};
    static const uint8_t letter_Q[7] = {14, 17, 17, 17, 21, 18, 13};
    static const uint8_t letter_R[7] = {30, 17, 17, 30, 20, 18, 17};
    static const uint8_t letter_S[7] = {15, 16, 16, 14, 1, 1, 30};
    static const uint8_t letter_T[7] = {31, 4, 4, 4, 4, 4, 4};
    static const uint8_t letter_U[7] = {17, 17, 17, 17, 17, 17, 14};
    static const uint8_t letter_V[7] = {17, 17, 17, 17, 10, 10, 4};
    static const uint8_t letter_W[7] = {17, 17, 17, 21, 21, 27, 17};
    static const uint8_t letter_X[7] = {17, 17, 10, 4, 10, 17, 17};
    static const uint8_t letter_Y[7] = {17, 17, 10, 4, 4, 4, 4};
    static const uint8_t letter_Z[7] = {31, 1, 2, 4, 8, 16, 31};
    static const uint8_t colon[7] = {0, 4, 4, 0, 4, 4, 0};
    static const uint8_t dot[7] = {0, 0, 0, 0, 0, 4, 4};
    static const uint8_t comma[7] = {0, 0, 0, 0, 0, 4, 8};
    static const uint8_t percent[7] = {17, 2, 4, 8, 16, 17, 0};
    static const uint8_t minus[7] = {0, 0, 0, 31, 0, 0, 0};

    if (c >= 'a' && c <= 'z') {
        c = (char)(c - 'a' + 'A');
    }

    switch (c) {
        case '0': return digit_0;
        case '1': return digit_1;
        case '2': return digit_2;
        case '3': return digit_3;
        case '4': return digit_4;
        case '5': return digit_5;
        case '6': return digit_6;
        case '7': return digit_7;
        case '8': return digit_8;
        case '9': return digit_9;
        case 'A': return letter_A;
        case 'B': return letter_B;
        case 'C': return letter_C;
        case 'D': return letter_D;
        case 'E': return letter_E;
        case 'F': return letter_F;
        case 'G': return letter_G;
        case 'H': return letter_H;
        case 'I': return letter_I;
        case 'J': return letter_J;
        case 'K': return letter_K;
        case 'L': return letter_L;
        case 'M': return letter_M;
        case 'N': return letter_N;
        case 'O': return letter_O;
        case 'P': return letter_P;
        case 'Q': return letter_Q;
        case 'R': return letter_R;
        case 'S': return letter_S;
        case 'T': return letter_T;
        case 'U': return letter_U;
        case 'V': return letter_V;
        case 'W': return letter_W;
        case 'X': return letter_X;
        case 'Y': return letter_Y;
        case 'Z': return letter_Z;
        case ':': return colon;
        case '.': return dot;
        case ',': return comma;
        case '%': return percent;
        case '-': return minus;
        case ' ': return blank;
        default: return blank;
    }
}

static esp_err_t oled_write_cmd(uint8_t cmd)
{
    uint8_t data[2] = {0x00, cmd};
    return i2c_master_write_to_device(OLED_I2C_PORT, s_oled_addr, data, sizeof(data), pdMS_TO_TICKS(100));
}

static esp_err_t oled_write_data(const uint8_t *data, size_t len)
{
    uint8_t page[1 + OLED_WIDTH];
    page[0] = 0x40;
    while (len > 0) {
        size_t chunk = len > OLED_WIDTH ? OLED_WIDTH : len;
        memcpy(&page[1], data, chunk);
        esp_err_t err = i2c_master_write_to_device(OLED_I2C_PORT, s_oled_addr, page, chunk + 1, pdMS_TO_TICKS(100));
        if (err != ESP_OK) {
            return err;
        }
        data += chunk;
        len -= chunk;
    }
    return ESP_OK;
}

static bool oled_probe_address(uint8_t addr)
{
    const uint8_t probe_byte = 0x00;
    esp_err_t err = i2c_master_write_to_device(OLED_I2C_PORT, addr, &probe_byte, 1, pdMS_TO_TICKS(50));
    return err == ESP_OK;
}

static void oled_clear(void)
{
    memset(s_oled_buf, 0, sizeof(s_oled_buf));
}

static void oled_draw_pixel(int x, int y, bool on)
{
    if (x < 0 || y < 0 || x >= OLED_WIDTH || y >= OLED_HEIGHT) {
        return;
    }
    size_t index = (size_t)x + (size_t)(y / 8) * OLED_WIDTH;
    uint8_t mask = (uint8_t)(1U << (y & 7));
    if (on) {
        s_oled_buf[index] |= mask;
    } else {
        s_oled_buf[index] &= (uint8_t)~mask;
    }
}

static void oled_fill_rect(int x, int y, int w, int h, bool on)
{
    for (int yy = y; yy < y + h; ++yy) {
        for (int xx = x; xx < x + w; ++xx) {
            oled_draw_pixel(xx, yy, on);
        }
    }
}

static void oled_draw_char(int x, int y, char c, int scale, bool on)
{
    const uint8_t *glyph = glyph_for(c);
    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 5; ++col) {
            bool bit_on = (glyph[row] & (1 << (4 - col))) != 0;
            if (!bit_on) {
                continue;
            }
            for (int sy = 0; sy < scale; ++sy) {
                for (int sx = 0; sx < scale; ++sx) {
                    oled_draw_pixel(x + col * scale + sx, y + row * scale + sy, on);
                }
            }
        }
    }
}

static void oled_draw_text(int x, int y, const char *text, int scale, bool on)
{
    int cursor = x;
    while (*text) {
        oled_draw_char(cursor, y, *text, scale, on);
        cursor += 6 * scale;
        ++text;
    }
}

static void oled_draw_hline(int x, int y, int w, bool on)
{
    for (int i = 0; i < w; ++i) {
        oled_draw_pixel(x + i, y, on);
    }
}

static void oled_draw_vline(int x, int y, int h, bool on)
{
    for (int i = 0; i < h; ++i) {
        oled_draw_pixel(x, y + i, on);
    }
}

static void oled_draw_rect(int x, int y, int w, int h, bool on)
{
    oled_draw_hline(x, y, w, on);
    oled_draw_hline(x, y + h - 1, w, on);
    oled_draw_vline(x, y, h, on);
    oled_draw_vline(x + w - 1, y, h, on);
}

static esp_err_t oled_flush(void)
{
    for (int page = 0; page < OLED_PAGES; ++page) {
        esp_err_t err = oled_write_cmd((uint8_t)(0xB0 | page));
        if (err != ESP_OK) return err;
        err = oled_write_cmd(0x00);
        if (err != ESP_OK) return err;
        err = oled_write_cmd(0x10);
        if (err != ESP_OK) return err;
        err = oled_write_data(&s_oled_buf[page * OLED_WIDTH], OLED_WIDTH);
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}

void oled_init(void)
{
    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = PIN_I2C_SDA,
        .scl_io_num = PIN_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    ESP_ERROR_CHECK(i2c_param_config(OLED_I2C_PORT, &cfg));
    ESP_ERROR_CHECK(i2c_driver_install(OLED_I2C_PORT, cfg.mode, 0, 0, 0));

    vTaskDelay(pdMS_TO_TICKS(50));

    /* probe common SSD1306 addresses: 0x3C, 0x3D, 0x3F */
    if (oled_probe_address(0x3C)) {
        s_oled_addr = 0x3C;
    } else if (oled_probe_address(0x3D)) {
        s_oled_addr = 0x3D;
    } else if (oled_probe_address(OLED_ADDR)) {
        s_oled_addr = OLED_ADDR;
    } else {
        ESP_LOGW(TAG, "No OLED found at 0x3C, 0x3D or 0x3F");
        return;
    }

    ESP_LOGI(TAG, "OLED address: 0x%02X", s_oled_addr);

    ESP_ERROR_CHECK(oled_write_cmd(0xAE));
    ESP_ERROR_CHECK(oled_write_cmd(0xD5));
    ESP_ERROR_CHECK(oled_write_cmd(0x80));
    ESP_ERROR_CHECK(oled_write_cmd(0xA8));
    ESP_ERROR_CHECK(oled_write_cmd(0x3F));
    ESP_ERROR_CHECK(oled_write_cmd(0xD3));
    ESP_ERROR_CHECK(oled_write_cmd(0x00));
    ESP_ERROR_CHECK(oled_write_cmd(0x40));
    ESP_ERROR_CHECK(oled_write_cmd(0x8D));
    ESP_ERROR_CHECK(oled_write_cmd(0x14));
    ESP_ERROR_CHECK(oled_write_cmd(0x20));
    ESP_ERROR_CHECK(oled_write_cmd(0x00));
    ESP_ERROR_CHECK(oled_write_cmd(0xA1));
    ESP_ERROR_CHECK(oled_write_cmd(0xC8));
    ESP_ERROR_CHECK(oled_write_cmd(0xDA));
    ESP_ERROR_CHECK(oled_write_cmd(0x12));
    ESP_ERROR_CHECK(oled_write_cmd(0x81));
    ESP_ERROR_CHECK(oled_write_cmd(0xCF));
    ESP_ERROR_CHECK(oled_write_cmd(0xD9));
    ESP_ERROR_CHECK(oled_write_cmd(0xF1));
    ESP_ERROR_CHECK(oled_write_cmd(0xDB));
    ESP_ERROR_CHECK(oled_write_cmd(0x40));
    ESP_ERROR_CHECK(oled_write_cmd(0xA4));
    ESP_ERROR_CHECK(oled_write_cmd(0xA6));
    ESP_ERROR_CHECK(oled_write_cmd(0xAF));

    oled_clear();
    ESP_ERROR_CHECK(oled_flush());
}

void oled_show_boot(void)
{
    oled_clear();
    oled_fill_rect(0, 0, OLED_WIDTH, OLED_HEIGHT, false);
    oled_draw_rect(0, 0, OLED_WIDTH, OLED_HEIGHT, true);
    oled_draw_text(10, 6, "OLED I2C", 2, true);
    oled_draw_text(10, 26, "SSD1306", 2, true);
    oled_draw_text(10, 46, "OK", 2, true);
    ESP_ERROR_CHECK(oled_flush());
    vTaskDelay(pdMS_TO_TICKS(1000));
}

void oled_render_ui(const controller_t *controller)
{
    static int s_page = 0;
    static TickType_t s_last_switch_tick = 0;

    TickType_t now_tick = xTaskGetTickCount();
    if (s_last_switch_tick == 0) {
        s_last_switch_tick = now_tick;
    } else if ((now_tick - s_last_switch_tick) >= pdMS_TO_TICKS(5000)) {
        s_page = (s_page + 1) % 3;
        s_last_switch_tick = now_tick;
    }

    oled_clear();
    oled_draw_rect(0, 0, OLED_WIDTH, OLED_HEIGHT, true);

    switch (s_page) {
        case 0:
        {
            char line[24];
            oled_draw_text(84, 6, "AC", 1, true);
            snprintf(line, sizeof(line), "U:%.0fV", controller->ac_voltage_v);
            oled_draw_text(6, 10, line, 1, true);
            snprintf(line, sizeof(line), "I:%.0fA", controller->ac_current_a);
            oled_draw_text(6, 28, line, 1, true);
            snprintf(line, sizeof(line), "F:%.0fHz", controller->frequency_hz);
            oled_draw_text(6, 46, line, 1, true);
            break;
        }
        case 1:
        {
            char line[24];
            oled_draw_text(84, 6, "DC", 1, true);
            snprintf(line, sizeof(line), "U:%.0fV", controller->dc_voltage_v);
            oled_draw_text(6, 18, line, 1, true);
            snprintf(line, sizeof(line), "A:%.0fA", controller->dc_current_a);
            oled_draw_text(6, 36, line, 1, true);
            break;
        }
        case 2:
        {
            char line[24];
            oled_draw_text(84, 6, "TEMP", 1, true);
            snprintf(line, sizeof(line), "T1:%.0fC", controller->temp_t1_c);
            oled_draw_text(6, 14, line, 1, true);
            snprintf(line, sizeof(line), "T2:%.0fC", controller->temp_t2_c);
            oled_draw_text(6, 32, line, 1, true);
            snprintf(line, sizeof(line), "Fs:%.0f%%", controller->fan_speed_percent);
            oled_draw_text(6, 50, line, 1, true);
            break;
        }
    }

    ESP_ERROR_CHECK(oled_flush());
}
