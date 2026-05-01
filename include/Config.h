#ifndef CONFIG_H
#define CONFIG_H

/* Global pin assignments (MKR Zero + MKR ETH Shield) */
#define PIN_LED_OUTPUT        2
/* Short press: wake OLED screensaver. Long hold (BUTTON_HOLD_MS): factory reset. */
#define PIN_RESET_BUTTON      6
#define PIN_ETHERNET_CS       5

/* Flicker limits (used by FlickerController and LedDriver).
   1 Hz achievable: direct TCC1 path selects prescaler (DIV2 needed at 1 Hz). */
#define MIN_FREQ_HZ          1
#define MAX_FREQ_HZ          500
#define DEFAULT_FREQ_HZ      50
#define MIN_DUTY_PERCENT     10
#define MAX_DUTY_PERCENT     90
#define DEFAULT_DUTY_PERCENT 50
#define MIN_INTENSITY_PERCENT 0
#define MAX_INTENSITY_PERCENT 100
#define DEFAULT_INTENSITY_PERCENT 100

/* PWM carrier for intensity and sinus mode (configurable in config) */
#define MIN_CARRIER_HZ        1000
#define MAX_CARRIER_HZ        50000
#define DEFAULT_CARRIER_HZ    20000

/* Timing */
#define BUTTON_HOLD_MS        10000
#define DISPLAY_UPDATE_MS     500
/* OLED powers down after this much time without set-button activity; press set to wake. */
#define DISPLAY_SCREENSAVER_IDLE_MS (5UL * 60UL * 1000UL)
#define SERIAL_LINE_TIMEOUT_MS 5000   /* Process buffered line after this ms with no new data */
#define PROTOCOL_CMD_LINE_BUFFER_MAX 128
#define PROTOCOL_PARSE_CMD_MAX       32
#define PROTOCOL_PARSE_ARGS_MAX      64

/* Network defaults */
#define SOCKET_PORT           23
#define HTTP_PORT             80

/* Dotted-quad "255.255.255.255" plus terminator */
#define IPV4_STRING_BUFFER_LEN 16

/* Frequency calibration table size (number of wizard steps / stored points) */
#define CAL_MAX_POINTS 8

/* ASCII shell response buffer size */
#define PROTOCOL_RESPONSE_MAX 128

#endif
