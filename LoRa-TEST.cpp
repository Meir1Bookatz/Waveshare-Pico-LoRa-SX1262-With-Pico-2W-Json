#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "sx126x.h"
struct timeval tp;
static uint8_t build_json(char* buf, size_t buf_size);
static void led_State(bool StateOn);
static void blink(int del);
static void blink(int times, int del);
const long interval = 500;

int main() {
    stdio_init_all();
    sleep_ms(1500);  // Give USB serial time to connect
    hard_assert(cyw43_arch_init() == PICO_OK);
    printf(
        "\n=== Pico 2W LoRa SX126x Test ===\nPress the button (GPIO %d) to "
        "send a JSON packet.\n\n",
        PIN_BUTTON);

    gpio_init(PIN_BUTTON);
    gpio_set_dir(PIN_BUTTON, GPIO_IN);
    gpio_pull_up(PIN_BUTTON);
    // Init LoRa
    sx126x_init();
    // Start listening immediately
    sx126x_start_rx();

    bool button_was_pressed = false;
    bool ledState = true;
    uint32_t previousMillis = 0;
    uint32_t currentMillis;
    bool passedInterval = false;
    while (true) {
        // ──────── blink ────────────────────────────────
        currentMillis = to_ms_since_boot(get_absolute_time());
        // if (passedInterval && passedInterval + 10000L) {
        // printf(
        //     "=================================================="
        //     "\npassedInterval[%d] = "
        //     "currentMillis[%d] - previousMillis[%d] >= "
        //     "interval[%d]\n==================================================",
        //     passedInterval, currentMillis, previousMillis, interval);
        // }
        if (currentMillis - previousMillis >= interval) {
            previousMillis = currentMillis;
            led_State(ledState);
            ledState = !ledState;
        }

        // Debounce
        bool button_pressed = !gpio_get(PIN_BUTTON);  // active LOW
        if (button_pressed && !button_was_pressed) {
            sleep_ms(20);
            if (!gpio_get(PIN_BUTTON)) {
                printf("\n[MAIN] Button pressed — sending JSON...\n");
                // Build JSON
                char json_buf[128];
                uint8_t json_len = build_json(json_buf, sizeof(json_buf));
                printf("[MAIN] Payload (%d bytes): %.*s\n", json_len,
                       (int)json_len, json_buf);
                blink(2, 150);  // Blink once before TX
                sx126x_send((const uint8_t*)json_buf, json_len);  // Send
                blink(2, 150);  // Blink twice to confirm TX done
                sx126x_start_rx();
                printf("[MAIN] Back in RX mode.\n");
            }
        }
        button_was_pressed = button_pressed;

        // ── RX polling ─────────────────────────────────────────────
        uint8_t rx_buf[256];
        uint8_t rx_len = 0;
        int result = sx126x_receive(rx_buf, &rx_len);
        //printf(" Raw JSON: [%s]\n", (char*)rx_buf);

        if (result == 1) {
            // Null-terminate so we can print as a string
            rx_buf[rx_len] = '\0';
            printf("\n[MAIN] *** Packet received! (%d bytes) ***\n", rx_len);
            printf("[MAIN] Raw JSON: [%s]\n[MAIN] length: [%d]", (char*)rx_buf, rx_len);
            printf("[MAIN] Parsing fields:\n");

            // Very simple field extraction without a JSON library.
            // Works for the fixed format we're sending.
            auto extract_str = [](const char* json, const char* key, char* out,
                                  size_t out_len) -> bool {
                const char* p = strstr(json, key);
                if (!p) return false;
                p += strlen(key);
                // Skip to the value (past the colon)
                while (*p == ':' || *p == ' ') p++;
                // Copy until delimiter
                size_t i = 0;
                bool in_str = (*p == '"');
                if (in_str) p++;
                while (*p && i < out_len - 1) {
                    if (in_str && *p == '"') break;
                    if (!in_str && (*p == ',' || *p == '}')) break;
                    out[i++] = *p++;
                }
                out[i] = '\0';
                return i > 0;
            };

            char val[32];
            if (extract_str((char*)rx_buf, "\"id\"", val, sizeof(val)))
                printf("  id   = %s\n", val);
            if (extract_str((char*)rx_buf, "\"temp\"", val, sizeof(val)))
                printf("  temp = %s °C\n", val);
            if (extract_str((char*)rx_buf, "\"hum\"", val, sizeof(val)))
                printf("  hum  = %s %%\n", val);
            if (extract_str((char*)rx_buf, "\"batt\"", val, sizeof(val)))
                printf("  batt = %s V\n", val);
            if (extract_str((char*)rx_buf, "\"rssi\"", val, sizeof(val)))
                printf("  rssi = %s dBm\n", val);
            // Blink to indicate a packet was received
            blink(3, 150);
            printf("[MAIN] Done processing packet.\n\n");
        }

        sleep_ms(10);  // Small yield to avoid hammering the bus

        // while (!stdio_usb_connected()) {
        //     blink(50);
        // }
        // // uart_puts(UART_ID, " Hello, UART0!\n");
        // printf("Enter 1 or 0 To Turn LED ON or OFF:\tLED State: %s\n",
        //        led_State == 1 ? "[1]On" : "[0]Off");
        // // uart_puts(uart0, " Hello, UART0!\n");
        // scanf("%d", &led_State);
        // led_State(led_State % 2 == 0 ? false : true);
        // sleep_ms(100);

        // blink(1, 80);
    }
    cyw43_arch_deinit();
    return 0;
}

static void led_State(bool StateOn) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, StateOn);
}
static void blink(int del) {
    led_State(true);
    sleep_ms(del);
    led_State(false);
    sleep_ms(del);
}

static void blink(int times, int del) {
    for (int i = 0; i < times; i++) {
        led_State(true);
        sleep_ms(del);
        led_State(false);
        sleep_ms(del);
    }
}

// ─── JSON payload
// ─────────────────────────────────────────────────────────────

// Builds a small JSON string with dummy sensor data into `buf`.
// Returns the length of the string (not including null terminator).
static uint8_t build_json(char* buf, size_t buf_size) {
    // Keep it short — LoRa payload is limited and we want fast airtime
    static uint32_t msg_id = 0;
    msg_id++;

    int len = snprintf(buf, buf_size,
                       "{"
                       "\"id\":%lu,"
                       "\"temp\":23.4,"
                       "\"hum\":61,"
                       "\"batt\":3.7,"
                       "\"rssi\":-45"
                       "}",
                       (unsigned long)msg_id);

    return (uint8_t)(len < 0 ? 0 : (len >= (int)buf_size ? buf_size - 1 : len));
}