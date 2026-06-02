#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/uart.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UART_SENSOR_KIND_UNKNOWN = 0,
    UART_SENSOR_KIND_AC,
    UART_SENSOR_KIND_DC,
} uart_sensor_kind_t;

typedef struct {
    uart_port_t port;
    int baud_rate;
    int tx_pin;
    int rx_pin;
    size_t rx_buffer_size;
} uart_sensor_reader_config_t;

typedef struct {
    uint8_t address;
    uart_sensor_kind_t kind;
    char payload[96];
    size_t payload_len;
    bool valid;
} uart_sensor_message_t;

typedef void (*uart_sensor_message_callback_t)(const uart_sensor_message_t *message, void *user_data);

esp_err_t uart_sensor_reader_init(const uart_sensor_reader_config_t *config);
esp_err_t uart_sensor_reader_deinit(void);
bool uart_sensor_reader_is_ac_address(uint8_t address);
bool uart_sensor_reader_is_dc_address(uint8_t address);
const char *uart_sensor_reader_kind_name(uart_sensor_kind_t kind);
bool uart_sensor_reader_parse_message(const char *line, uart_sensor_message_t *message);
esp_err_t uart_sensor_reader_poll(TickType_t timeout_ticks,
                                  uart_sensor_message_callback_t callback,
                                  void *user_data);

#ifdef __cplusplus
}
#endif
