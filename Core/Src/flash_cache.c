/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Ha Thach for Adafruit Industries
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

#include "flash_cache.h"

#include <string.h>
#include <stdio.h>

#include "quadspi.h"

#define FLASH_CACHE_SIZE          4096
#define FLASH_CACHE_INVALID_ADDR  0xffffffff

static uint32_t _fl_addr = FLASH_CACHE_INVALID_ADDR;
static uint8_t _fl_buf[FLASH_CACHE_SIZE] __attribute__((aligned(4)));

board_flash_err_t board_flash_flush(void)
{
  if ( _fl_addr == FLASH_CACHE_INVALID_ADDR ) {
    return BFLASH_RET_OK;
  }
  // puts("flash flush");
  if (MX_QUADSPI_Erase(_fl_addr, FLASH_CACHE_SIZE) != FLASH_CACHE_SIZE) {
    return BFLASH_RET_NOERASE;
  }
  if (MX_QUADSPI_Write(_fl_buf, _fl_addr, FLASH_CACHE_SIZE) != FLASH_CACHE_SIZE) {
    return BFLASH_RET_NOWRITE;
  }

  _fl_addr = FLASH_CACHE_INVALID_ADDR;
  return BFLASH_RET_OK;
}

board_flash_err_t board_flash_write (uint32_t dst, void const *src, uint32_t len)
{
  uint32_t new_addr = dst & ~(FLASH_CACHE_SIZE - 1);

  if ( new_addr != _fl_addr )
  {
    board_flash_err_t r = board_flash_flush();
    if (r != BFLASH_RET_OK) {
      return r;
    }

    _fl_addr = new_addr;
    if (MX_QUADSPI_Read(_fl_buf, new_addr, FLASH_CACHE_SIZE) != FLASH_CACHE_SIZE) {
      return BFLASH_RET_NOREAD;
    }
  }

  memcpy(_fl_buf + (dst & (FLASH_CACHE_SIZE - 1)), src, len);
  return BFLASH_RET_OK;
}
