#include "espressif/esp_common.h"
#include "FreeRTOS.h"
#include "task.h"
#include "esp/uart.h"
#include <stdio.h>
#include <stdint.h>

#include "ws2812_i2s/ws2812_i2s.h"

extern volatile uint32_t dma_isr_counter;

static void demo(void *pvParameters)
{
    const uint32_t pixels_number = 30;
    ws2812_pixel_t pixels[pixels_number];

    ws2812_i2s_init(pixels_number);

    for (int i = 0; i < pixels_number; i++) {
        pixels[i].red = 0xFF;
        pixels[i].green = 0;
        pixels[i].blue = 0;
    }

    while (1) {
        vTaskDelay(1000 / portTICK_RATE_MS);
        printf("isr counter: %d\n", dma_isr_counter);

        ws2812_i2s_update(pixels);
    }
}

void user_init(void)
{
    uart_set_baud(0, 115200);

    sdk_wifi_set_opmode(NULL_MODE);

    xTaskCreate(&demo, (signed char *)"ws2812_i2s", 256, NULL, 10, NULL);
}
