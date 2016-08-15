#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "esp8266.h"

#include "fcntl.h"
#include "unistd.h"

#include <stdio.h>
#include <stdint.h>

#include "spiffs.h"
#include "esp_spiffs.h"
#include "i2s_dma/i2s_dma.h"

typedef struct __attribute__((packed)) {
    uint8_t ignore[40];
    uint32_t data_size;
    uint8_t data[];
} dumb_wav_header_t;

volatile uint32_t dma_isr_counter = 0;
volatile int dma_block_processed = -1;


#define DMA_BUFFER_SIZE    2048
#define DMA_BUFFER_NUMBER  2

static dma_descriptor_t dma_block_list[DMA_BUFFER_NUMBER];

static uint8_t dma_buffer[DMA_BUFFER_NUMBER][DMA_BUFFER_SIZE];

static inline void init_descriptors_list()
{
    for (int i = 0; i < DMA_BUFFER_NUMBER; i++) {
        dma_block_list[i].owner = 1;
        dma_block_list[i].eof = 1;
        dma_block_list[i].sub_sof = 0;
        dma_block_list[i].unused = 0;
        dma_block_list[i].buf_ptr = dma_buffer[i];
        dma_block_list[i].datalen = 0;
        dma_block_list[i].blocksize = DMA_BUFFER_SIZE;
        if (i == (DMA_BUFFER_NUMBER - 1)) {
            dma_block_list[i].next_link_ptr = &dma_block_list[0];
        } else {
            dma_block_list[i].next_link_ptr = &dma_block_list[i + 1];
        }
    }
}

static void dma_isr_handler(void)
{
    if (i2s_dma_is_eof_interrupt()) {
        /* dma_descriptor_t *descr = i2s_dma_get_eof_descriptor(); */
        /* if (descr == &dma_block_list[0]) { */
        /*     dma_block_processed = 0; */
        /* } else if (descr == &dma_block_list[1]) { */
        /*     dma_block_processed = 1; */
        /* } else { */
        /*     dma_block_processed = -1; */
        /* } */
        if (dma_block_processed == 0) {
            dma_block_processed = 1;
        } else {
            dma_block_processed = 0;
        }
        dma_isr_counter++;
    }
    i2s_dma_clear_interrupt();
}

static bool play_data(int fd)
{
    static int last_played_block = -1;
    int curr_block = 0;

    while (last_played_block != dma_block_processed) {}

    if (last_played_block == -1) {
        curr_block = 0;
    } else if (last_played_block == 0) {
        curr_block = 1;
    } else if (last_played_block == 1) {
        curr_block = 0;
    }
    
    int read_bytes = read(fd, dma_buffer[curr_block], DMA_BUFFER_SIZE);
    if (read_bytes <= 0) {
        return false;
    }
    /* printf("read %d bytes\n", read_bytes); */
    dma_block_list[curr_block].datalen = read_bytes;
    
    if (last_played_block == -1) {  // it is a start
        i2s_dma_start(dma_block_list);
    }

    last_played_block = curr_block;
    return true;
}

void play_task(void *pvParameters)
{
    esp_spiffs_init();
    if (esp_spiffs_mount() != SPIFFS_OK) {
        printf("Error mount SPIFFS\n");
    }

    // 2 channels 16 bits 44100 Hz
    i2s_clock_div_t clock_div = i2s_get_clock_div(44100 * 2 * 16);
    i2s_pins_t i2s_pins = {.data = true, .clock = true, .ws = true};

    printf("i2s clock dividers, bclk=%d, clkm=%d\n",
            clock_div.bclk_div, clock_div.clkm_div);

    i2s_dma_init(dma_isr_handler, clock_div, i2s_pins);

    init_descriptors_list();

    while (1) {
        dumb_wav_header_t wav_header;

        int fd = open("sample.wav", O_RDONLY);
        if (fd < 0) {
            printf("Error opening file\n");
            break;
        }

        read(fd, (void*)&wav_header, sizeof(wav_header));
        printf("wav data size: %d\n", wav_header.data_size);

        while (play_data(fd)) {};

        printf("interrupt counter: %d\n", dma_isr_counter);

        close(fd);
        printf("\n\n");
    }
}

void user_init(void)
{
    uart_set_baud(0, 115200);

    xTaskCreate(play_task, (signed char *)"test_task", 1024, NULL, 2, NULL);
}
