#ifndef CONFIG_H
#define CONFIG_H

/* MKR Zero + MKR ETH shield */
#define PIN_LED_OUTPUT_A      2   /* TCC1 WO[0] */
#define PIN_LED_OUTPUT_B      3   /* TCC1 WO[1] — same waveform as A */
#define PIN_RESET_BUTTON      6   /* OLED wake + factory reset (long hold) */
#define PIN_ETHERNET_CS       5

/* Envelope / modulation frequency (Hz), flicker and sinus */
#define MIN_FREQ_HZ          1
#define MAX_FREQ_HZ          500
#define DEFAULT_FREQ_HZ      50
#define MIN_DUTY_PERCENT     10
#define MAX_DUTY_PERCENT     90
#define DEFAULT_DUTY_PERCENT 50
#define MIN_INTENSITY_PERCENT 0
#define MAX_INTENSITY_PERCENT 100
#define DEFAULT_INTENSITY_PERCENT 100

/* Fast PWM carrier for intensity shaping (constant + sinus); ignored in pure flicker envelope path */
#define MIN_CARRIER_HZ        1000
#define MAX_CARRIER_HZ        50000
#define DEFAULT_CARRIER_HZ    20000

#define BUTTON_HOLD_MS        10000
#define DISPLAY_UPDATE_MS     500
#define MIN_DISPLAY_SCREENSAVER_S  1
#define MAX_DISPLAY_SCREENSAVER_S  600
#define DEFAULT_DISPLAY_SCREENSAVER_S 10
#define SERIAL_LINE_TIMEOUT_MS 5000
#define PROTOCOL_CMD_LINE_BUFFER_MAX 128
#define PROTOCOL_PARSE_CMD_MAX       32
#define PROTOCOL_PARSE_ARGS_MAX      64

#define SOCKET_PORT           23
#define HTTP_PORT             80

#define MDNS_PORT                 5353
#define MDNS_HOSTNAME_PREFIX      "flicker-"
#define MDNS_HOSTNAME_PREFIX_LEN  8
#define MDNS_HOSTNAME_SUFFIX_LEN  6
#define MDNS_HOSTNAME_BUFFER_LEN  (MDNS_HOSTNAME_PREFIX_LEN + MDNS_HOSTNAME_SUFFIX_LEN + 1)
#define MDNS_HTTP_SERVICE_NAME    "Flicker._http"

#define IPV4_STRING_BUFFER_LEN 16

#define CAL_MAX_POINTS 8

#define PROTOCOL_RESPONSE_MAX 128

#endif
