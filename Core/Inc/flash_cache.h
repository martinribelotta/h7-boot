#ifndef __FLASH_CACHE_H__
#define __FLASH_CACHE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BFLASH_RET_OK = 0,
    BFLASH_RET_NOERASE,
    BFLASH_RET_NOWRITE,
    BFLASH_RET_NOREAD,
} board_flash_err_t;

board_flash_err_t board_flash_flush(void);
board_flash_err_t board_flash_write (uint32_t dst, void const *src, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif // __FLASH_CACHE_H__
