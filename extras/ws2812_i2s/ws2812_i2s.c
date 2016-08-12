/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 sheinz (https://github.com/sheinz)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "ws2812_i2s.h"
#include "i2s_dma.h"

#include <string.h>
#include <malloc.h>

#define WS2812_I2S_DEBUG

#ifdef WS2812_I2S_DEBUG
#include <stdio.h>
#define debug(fmt, ...) printf("%s" fmt "\n", "ws2812_i2s: ", ## __VA_ARGS__);
#else
#define debug(fmt, ...)
#endif

#define MAX_DMA_BLOCK_SIZE      4095  // 12 bits
#define DMA_PIXEL_SIZE          12    // each color takes 4 bytes
#define WS2812_ZEROES_LENGTH    16

static uint8_t i2s_dma_zero_buf[WS2812_ZEROES_LENGTH] = {0};

// debug
volatile uint32_t dma_isr_counter = 0;

static void dma_isr_handler(void)
{
    dma_isr_counter++;
    i2s_dma_clear_interrupt();
    if (i2s_dma_is_eof_interrupt()) {
        /* dma_descriptor_t *eof_descr = i2s_dma_get_eof_descriptor(); */
    }
}

/**
 * Form linked list of descriptors (dma blocks).
 * The last two blocks are zero blocks. The form an end loop.
 * In the end loop I2S DMA idle loop untill new data is feed.
 */
static inline void init_descriptors_list(dma_descriptor_t *dma_blocks, 
        uint32_t size, uint32_t total_dma_buf_size)
{
    for (int i = 0; i < size; i++) {
        dma_blocks[i].owner = 1;
        dma_blocks[i].eof = 0;
        dma_blocks[i].sub_sof = 0;
        dma_blocks[i].unused = 0;

        if (total_dma_buf_size >= MAX_DMA_BLOCK_SIZE) {
            dma_blocks[i].datalen = MAX_DMA_BLOCK_SIZE;
            dma_blocks[i].blocksize = MAX_DMA_BLOCK_SIZE;
            total_dma_buf_size -= MAX_DMA_BLOCK_SIZE;
        } else {
            dma_blocks[i].datalen = total_dma_buf_size;
            dma_blocks[i].blocksize = total_dma_buf_size;
            total_dma_buf_size = 0;
        }
        if (dma_blocks[i].datalen) {  // allocate memory only for actuall data
            dma_blocks[i].buf_ptr = malloc(dma_blocks[i].datalen);
            memset(dma_blocks[i].buf_ptr, 0, dma_blocks[i].datalen);
        } else {
            dma_blocks[i].buf_ptr = i2s_dma_zero_buf;
            dma_blocks[i].datalen = WS2812_ZEROES_LENGTH;
            dma_blocks[i].blocksize = WS2812_ZEROES_LENGTH;
        }
        if (i == (size - 1)) { // is it the last block
            // the two last zero block should make a loop
            dma_blocks[i].next_link_ptr = 0;//&dma_blocks[i-1];
        } else {
            dma_blocks[i].next_link_ptr = &dma_blocks[i+1];
        }
    }

    // first zero block should trigger interrupt
    dma_blocks[size - 2].eof = 1;
}

void ws2812_i2s_init(uint32_t pixels_number)
{
    uint32_t total_dma_buf_size = pixels_number * DMA_PIXEL_SIZE;
    uint32_t blocks_number = total_dma_buf_size / MAX_DMA_BLOCK_SIZE;

    if (total_dma_buf_size % MAX_DMA_BLOCK_SIZE) {
        blocks_number += 1;
    }

    blocks_number += 2;  // two extra zero blocks

    debug("allocating %d dma blocks\n", blocks_number);

    dma_descriptor_t *dma_blocks = (dma_descriptor_t*)malloc(
            blocks_number * sizeof(dma_descriptor_t));

    init_descriptors_list(dma_blocks, blocks_number, total_dma_buf_size);

    i2s_clock_div_t clock_div = i2s_get_clock_div(3333333);
    i2s_pins_t i2s_pins = {.data = true, .clock = false, .ws = false};

    debug("i2s clock dividers, bclk=%d, clkm=%d\n", 
            clock_div.bclk_div, clock_div.clkm_div);

    i2s_dma_init(dma_blocks, dma_isr_handler, clock_div, i2s_pins);
    i2s_dma_start();
}

