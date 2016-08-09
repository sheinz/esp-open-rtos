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
#include "ws2812_dma.h"

#include <string.h>
#include <stdio.h>

static struct SLCDescriptor i2sBufDescOut;
static struct SLCDescriptor i2sBufDescZeroes;

#define WS_BLOCKSIZE 4000

static uint32_t i2sZeroes[32];
static uint32_t i2sBlock[WS_BLOCKSIZE/4];

void ws2812_i2s_init()
{
    memset(i2sZeroes, 0, 32);
    memset(i2sBlock, 0b10001110, WS_BLOCKSIZE);

    i2sBufDescOut.flags = SLC_DESCRIPTOR_FLAGS(WS_BLOCKSIZE, WS_BLOCKSIZE,
            0, 0, 1);
    i2sBufDescOut.buf_ptr = (uint32_t)i2sBlock;
    i2sBufDescOut.next_link_ptr = (uint32_t)&i2sBufDescZeroes;
    printf("desc out flags: %x\n", i2sBufDescOut.flags);

    i2sBufDescZeroes.flags = SLC_DESCRIPTOR_FLAGS(32, 32, 0, 0, 1);
    i2sBufDescZeroes.buf_ptr = (uint32_t)i2sZeroes;
    i2sBufDescZeroes.next_link_ptr = (uint32_t)&i2sBufDescOut;
    printf("desc zero flags: %x\n", i2sBufDescZeroes.flags);

    ws2812_dma_init(&i2sBufDescOut);
}

