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
#include <stdio.h>

static dma_descriptor_t i2sBufDescOut;
static dma_descriptor_t i2sBufDescZeroes;

#define WS_BLOCKSIZE 4000

static uint32_t i2sZeroes[32];
static uint32_t i2sBlock[WS_BLOCKSIZE/4];

void ws2812_i2s_init()
{
    memset(i2sZeroes, 0, 32);
    memset(i2sBlock, 0b10001110, WS_BLOCKSIZE);

    i2sBufDescOut.owner = 1;
    i2sBufDescOut.eof = 0;
    i2sBufDescOut.sub_sof = 0;
    i2sBufDescOut.unused = 0;
    i2sBufDescOut.datalen = WS_BLOCKSIZE;
    i2sBufDescOut.blocksize = WS_BLOCKSIZE;
    i2sBufDescOut.buf_ptr = i2sBlock;
    i2sBufDescOut.next_link_ptr = &i2sBufDescZeroes;

    i2sBufDescZeroes.owner = 1;
    i2sBufDescZeroes.eof = 0;
    i2sBufDescZeroes.sub_sof = 0;
    i2sBufDescZeroes.unused = 0;
    i2sBufDescZeroes.datalen = 32;
    i2sBufDescZeroes.blocksize = 32;
    i2sBufDescZeroes.buf_ptr = i2sZeroes;
    i2sBufDescZeroes.next_link_ptr = &i2sBufDescOut;

    i2s_pins_t i2s_pins = { .data = true, .clock = false, .ws = false};
    i2s_clock_div_t clock_div = i2s_get_clock_div(3333333);  // 3.333 MHz

    i2s_dma_init(&i2sBufDescOut, NULL, clock_div, i2s_pins);
}

