/* LwIP SNTP example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "esp_sntp.h"
#include "led_strip.h"
#include "driver/rmt.h"
#include "protocol_examples_common.h"

#define RMT_TX_CHANNEL RMT_CHANNEL_0

static const char *TAG = "example";

bool ever_set;
struct timeval last_set;
void time_sync_notification_cb(struct timeval *tv)
{
    ever_set = true;
    gettimeofday(&last_set, NULL);
}

// Measure the delay from GPS PPS to this thing's PPS. A
// *POSITIVE* offset means that this thing is LATE.
#define PPS_OFFSET_MS 14
#define US_IN_S (1000000)
#define RED 1,0,0
#define GREEN 0,1,0
#define CYAN 0,1,1
#define BLACK 0,0,0
#define led_set(strip, arg) do { strip->set_pixel(strip, 0, arg); strip->refresh(strip, 100); } while(0)
// Pin settings for QT Py ESP32-S2

#if CONFIG_SNTP_BOARD_TYPE_QTPY
#define NEOPIXEL_PWR (38)
#define NEOPIXEL (39) // AKA MTCK
#define GPIO_PPS (18) // Silk "A0"
#define GPIO_PPM (17) // Silk "A1"
#define GPIO_PPH (9) // Silk "A2"
#define GPIO_PPX (8) // Silk "A3"
#elif CONFIG_SNTP_BOARD_TYPE_FEATHER
#define NEOPIXEL_PWR (21)
#define NEOPIXEL (33)
#define GPIO_PPS (18) // Silk "A0"
#define GPIO_PPM (17) // Silk "A1"
#define GPIO_PPH (16) // Silk "A2"
#define GPIO_PPX (15) // Silk "A3"
#else
#error Unknown board type
#endif

_Static_assert(GPIO_PPS < 32, "Pin must be on first port");
_Static_assert(GPIO_PPM < 32, "Pin must be on first port");
_Static_assert(GPIO_PPH < 32, "Pin must be on first port");
_Static_assert(GPIO_PPX < 32, "Pin must be on first port");

void app_main(void)
{
    {
        gpio_config_t setting = {
            .pin_bit_mask = (1ull << GPIO_PPS) | (1ull << GPIO_PPM) | (1ull << GPIO_PPH) | (1ull << NEOPIXEL_PWR),
            .mode = GPIO_MODE_OUTPUT,
        };
        gpio_config(&setting);
    }

    gpio_set_level(NEOPIXEL_PWR, true);

    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(NEOPIXEL, RMT_TX_CHANNEL);
    // set counter clock to 40MHz
    config.clk_div = 2;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

    // install ws2812 driver
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(1, (led_strip_dev_t)config.channel);
    led_strip_t *strip = led_strip_new_rmt_ws2812(&strip_config);
    if (!strip) {
        ESP_LOGE(TAG, "install WS2812 driver failed");
    }

    led_set(strip, RED);

    ESP_ERROR_CHECK( nvs_flash_init() );
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK( esp_event_loop_create_default() );

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, CONFIG_SNTP_SERVER_NAME);
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
    sntp_init();

    while(true) {
        struct timeval tv;
        gettimeofday(&tv, NULL);

        // apply the OFFSET
        tv.tv_usec += PPS_OFFSET_MS * 1000;
        if(tv.tv_usec < 0) {
            tv.tv_sec -= 1;
            tv.tv_usec += US_IN_S;
        } else if(tv.tv_usec > US_IN_S) {
            tv.tv_sec += 1;
            tv.tv_usec -= US_IN_S;
        }

        const unsigned duty = US_IN_S/10;
        bool state = tv.tv_usec < duty;

        if (state) {
            bool top_of_minute = tv.tv_sec % 60 == 0;
            bool top_of_hour = tv.tv_sec % 3600 == 0;
            bool ppx = !top_of_minute && !top_of_hour;
            uint32_t to_set = (1u << GPIO_PPS);
            uint32_t to_clear = 0;

            if(top_of_minute) {
                to_set |= (1u << GPIO_PPM);
            } else {
                to_clear |= (1u << GPIO_PPM);
            }

            if(top_of_hour) {
                to_set |= (1u << GPIO_PPH);
            } else {
                to_clear |= (1u << GPIO_PPH);
            }

            if(ppx) {
                to_set |= (1u << GPIO_PPX);
            } else {
                to_clear |= (1u << GPIO_PPX);
            }


            // set all pins at the same moment
            GPIO.out_w1tc = to_clear;
            GPIO.out_w1ts = to_set;

            gpio_set_level(GPIO_PPS, state);
            if(last_set.tv_sec < tv.tv_sec && last_set.tv_sec > tv.tv_sec - 10) {
                led_set(strip, CYAN);
            } else if(ever_set) {
                led_set(strip, GREEN);
            } else {
                led_set(strip, RED);
            }
        } else {
            uint32_t to_clear = (1u << GPIO_PPS) | (1u << GPIO_PPM) | (1u << GPIO_PPH) | (1u << GPIO_PPX);
            GPIO.out_w1ts = to_clear;
            led_set(strip, BLACK);
        }

        unsigned long sleep_us = 1000000 - tv.tv_usec;
        if(tv.tv_usec < duty) {
            sleep_us = duty - tv.tv_usec;
        }
        usleep(sleep_us);
    }
}
