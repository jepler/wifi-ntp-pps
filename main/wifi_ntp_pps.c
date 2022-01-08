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

void time_sync_notification_cb(struct timeval *tv)
{
    ever_set = true;
}

#define RED 1,0,0
#define GREEN 0,1,0
#define BLACK 0,0,0
#define led_set(strip, arg) do { strip->set_pixel(strip, 0, arg); strip->refresh(strip, 100); } while(0)
// Pin settings for QT Py ESP32-S2

#if CONFIG_SNTP_BOARD_TYPE_QTPY
#define NEOPIXEL_PWR (38)
#define NEOPIXEL (39) // AKA MTCK
#define GPIO_PPS (18) // Silk "A0"
#elif CONFIG_SNTP_BOARD_TYPE_FEATHER
#define NEOPIXEL_PWR (21)
#define NEOPIXEL (33)
#define GPIO_PPS (18) // Silk "A0"
#else
#error Unknown board type
#endif

void app_main(void)
{
    {
        gpio_config_t setting = {
            .pin_bit_mask = (1ull << GPIO_PPS) | (1ull << NEOPIXEL_PWR),
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
        const unsigned duty = 100000;
        bool state = tv.tv_usec < duty;
        gpio_set_level(GPIO_PPS, state);

        if (state) {
            if(ever_set) {
                led_set(strip, GREEN);
            } else {
                led_set(strip, RED);
            }
        } else {
            led_set(strip, BLACK);
        }

        unsigned long sleep_us = 1000000 - tv.tv_usec;
        if(tv.tv_usec < duty) {
            sleep_us = duty - tv.tv_usec;
        }
        usleep(sleep_us);
    }
}
