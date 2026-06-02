#pragma once

#include "app_state.h"
#include "driver/i2c.h"

/* Run a quick I2C scan on the given port and log found addresses */
void i2c_scan_and_log(i2c_port_t port);

void oled_init(void);
void oled_show_boot(void);
void oled_render_ui(const controller_t *controller);
