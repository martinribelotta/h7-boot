#ifndef __FLASH_CACHE_H__
#define __FLASH_CACHE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void board_flash_flush(void);
void board_flash_write (uint32_t dst, void const *src, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif // __FLASH_CACHE_H__
