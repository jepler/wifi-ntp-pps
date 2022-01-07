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
#include "protocol_examples_common.h"
#include "esp_sntp.h"

static const char *TAG = "example";

bool ever_set;

void time_sync_notification_cb(struct timeval *tv)
{
    ever_set = true;
}

#define GPIO_PPS (18) // QT Py ESP32-S2 Silk "A0"

void app_main(void)
{
    {
        gpio_config_t setting = {
            .pin_bit_mask = (1ull << GPIO_PPS),
            .mode = GPIO_MODE_OUTPUT,
        };
        gpio_config(&setting);
    }

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
        unsigned duty = ever_set ? 100000 : 500000;
        bool state = tv.tv_usec < duty;
        gpio_set_level(GPIO_PPS, state);

        unsigned long sleep_us = 1000000 - tv.tv_usec;
        if(tv.tv_usec < duty) {
            sleep_us = duty - tv.tv_usec;
        }
        usleep(sleep_us);
    }
}
