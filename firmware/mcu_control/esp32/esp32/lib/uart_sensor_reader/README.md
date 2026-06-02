# uart_sensor_reader

Standalone ESP-IDF / PlatformIO helper for reading framed data from UART and routing it by logical sensor address.

## Supported addresses

- `0x35` - AC data
- `0x48` - DC data
- `0x49` - DC data

## Frame format

The library expects text frames ending with a newline, for example:

```text
35,230.5,2.4,50.0
48,24.1,9.8
49,24.0,9.7
```

The first token is treated as a hexadecimal address. The rest of the line is returned as raw payload.

## API

- `uart_sensor_reader_init(...)`
- `uart_sensor_reader_deinit()`
- `uart_sensor_reader_poll(...)`
- `uart_sensor_reader_parse_message(...)`
- `uart_sensor_reader_is_ac_address(...)`
- `uart_sensor_reader_is_dc_address(...)`

## Notes

- This library does not change `main.c`.
- It does not assume a specific payload structure after the address.
- If you want, I can add a small parser for the AC and DC payload fields in a second step.
