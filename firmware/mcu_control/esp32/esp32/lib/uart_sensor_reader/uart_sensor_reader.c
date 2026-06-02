#include "uart_sensor_reader.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"

#define UART_SENSOR_READER_LINE_MAX 128
#define UART_SENSOR_READER_CHUNK_MAX 64

static const char *TAG = "uart_sensor_reader";

static uart_sensor_reader_config_t s_config;
static bool s_initialized = false;
static char s_line_buffer[UART_SENSOR_READER_LINE_MAX];
static size_t s_line_length = 0;

static char *trim_left(char *text)
{
    while (*text != '\0' && isspace((unsigned char)*text)) {
        ++text;
    }
    return text;
}

static void trim_right(char *text)
{
    size_t length = strlen(text);
    while (length > 0 && isspace((unsigned char)text[length - 1])) {
        text[length - 1] = '\0';
        --length;
    }
}

static bool parse_u8_hex(const char *text, uint8_t *value)
{
    if (text == NULL || value == NULL) {
        return false;
    }

    while (*text != '\0' && isspace((unsigned char)*text)) {
        ++text;
    }

    if (*text == '\0') {
        return false;
    }

    char *end = NULL;
    unsigned long parsed = strtoul(text, &end, 16);
    if (end == text || parsed > 0xFFUL) {
        return false;
    }

    while (*end != '\0' && isspace((unsigned char)*end)) {
        ++end;
    }

    if (*end != '\0') {
        return false;
    }

    *value = (uint8_t)parsed;
    return true;
}

static uart_sensor_kind_t kind_from_address(uint8_t address)
{
    if (address == 0x35) {
        return UART_SENSOR_KIND_AC;
    }
    if (address == 0x48 || address == 0x49) {
        return UART_SENSOR_KIND_DC;
    }
    return UART_SENSOR_KIND_UNKNOWN;
}

bool uart_sensor_reader_is_ac_address(uint8_t address)
{
    return address == 0x35;
}

bool uart_sensor_reader_is_dc_address(uint8_t address)
{
    return address == 0x48 || address == 0x49;
}

const char *uart_sensor_reader_kind_name(uart_sensor_kind_t kind)
{
    switch (kind) {
        case UART_SENSOR_KIND_AC:
            return "AC";
        case UART_SENSOR_KIND_DC:
            return "DC";
        default:
            return "UNKNOWN";
    }
}

esp_err_t uart_sensor_reader_init(const uart_sensor_reader_config_t *config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (config->rx_buffer_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    uart_config_t uart_cfg = {
        .baud_rate = config->baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_param_config(config->port, &uart_cfg));
    ESP_ERROR_CHECK(uart_set_pin(config->port,
                                 config->tx_pin,
                                 config->rx_pin,
                                 UART_PIN_NO_CHANGE,
                                 UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(config->port,
                                        (int)config->rx_buffer_size,
                                        0,
                                        0,
                                        NULL,
                                        0));

    s_config = *config;
    s_initialized = true;
    s_line_length = 0;
    s_line_buffer[0] = '\0';

    ESP_LOGI(TAG, "UART initialized on port %d at %d baud", (int)s_config.port, s_config.baud_rate);
    return ESP_OK;
}

esp_err_t uart_sensor_reader_deinit(void)
{
    if (!s_initialized) {
        return ESP_OK;
    }

    esp_err_t err = uart_driver_delete(s_config.port);
    if (err == ESP_OK) {
        s_initialized = false;
        s_line_length = 0;
        s_line_buffer[0] = '\0';
    }
    return err;
}

bool uart_sensor_reader_parse_message(const char *line, uart_sensor_message_t *message)
{
    if (line == NULL || message == NULL) {
        return false;
    }

    char temp[UART_SENSOR_READER_LINE_MAX];
    size_t input_len = strnlen(line, sizeof(temp) - 1);
    if (input_len == 0 || input_len >= sizeof(temp)) {
        return false;
    }

    memcpy(temp, line, input_len);
    temp[input_len] = '\0';

    char *text = trim_left(temp);
    trim_right(text);
    if (*text == '\0') {
        return false;
    }

    char *separator = strpbrk(text, ",:;\t ");
    if (separator == NULL) {
        return false;
    }

    *separator = '\0';
    char *payload = trim_left(separator + 1);
    trim_right(text);
    trim_right(payload);

    uint8_t address = 0;
    if (!parse_u8_hex(text, &address)) {
        return false;
    }

    memset(message, 0, sizeof(*message));
    message->address = address;
    message->kind = kind_from_address(address);
    message->valid = true;

    size_t payload_len = strnlen(payload, sizeof(message->payload) - 1);
    memcpy(message->payload, payload, payload_len);
    message->payload[payload_len] = '\0';
    message->payload_len = payload_len;
    return true;
}

esp_err_t uart_sensor_reader_poll(TickType_t timeout_ticks,
                                  uart_sensor_message_callback_t callback,
                                  void *user_data)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t chunk[UART_SENSOR_READER_CHUNK_MAX];
    int read_len = uart_read_bytes(s_config.port, chunk, sizeof(chunk), timeout_ticks);
    if (read_len < 0) {
        return ESP_FAIL;
    }

    for (int index = 0; index < read_len; ++index) {
        char character = (char)chunk[index];

        if (character == '\r') {
            continue;
        }

        if (character == '\n') {
            if (s_line_length > 0) {
                s_line_buffer[s_line_length] = '\0';
                if (callback != NULL) {
                    uart_sensor_message_t message;
                    if (uart_sensor_reader_parse_message(s_line_buffer, &message)) {
                        callback(&message, user_data);
                    }
                }
                s_line_length = 0;
                s_line_buffer[0] = '\0';
            }
            continue;
        }

        if (s_line_length < sizeof(s_line_buffer) - 1) {
            s_line_buffer[s_line_length++] = character;
            s_line_buffer[s_line_length] = '\0';
        } else {
            s_line_length = 0;
            s_line_buffer[0] = '\0';
        }
    }

    return ESP_OK;
}
