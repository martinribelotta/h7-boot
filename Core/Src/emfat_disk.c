#include "emfat.h"

#include "quadspi.h"
#include "flash_cache.h"
#include "uf2.h"

#include "usbd_def.h"

#include <stdio.h>

emfat_t emfat;

#define CMA_TIME EMFAT_ENCODE_CMA_TIME(1, 12, 2020, 13, 0, 0)
#define CMA                                                                                                            \
    {                                                                                                                  \
        CMA_TIME, CMA_TIME, CMA_TIME                                                                                   \
    }
#define ROOT                                                                                                           \
    {                                                                                                                  \
        "", true, 0, 0, 0, 0, 0, CMA, NULL, NULL                                                                       \
    }
#define ARRAY_FILE(n, lvl, off, a)                                                                                     \
    {                                                                                                                  \
        n, false, lvl, off, sizeof(a) - 1, sizeof(a) - 1, (size_t)a, CMA, rofile_read_proc, NULL                       \
    }
#define BIN_DYN_FILE(n, lvl, off, rd, wr)                                                                              \
    {                                                                                                                  \
        n, false, lvl, off, 0, 0, 0, CMA, rd, wr                                                                       \
    }
#define ENDROOT                                                                                                        \
    {                                                                                                                  \
        NULL                                                                                                           \
    }

static const uint8_t info_uf2_data[] = //
    "UF2 Bootloader v1.1.3 SFA\r\n"
    "Model: STM32H7 Minitor\r\n"
    "Board-ID: STM32H7 Generic\r\n"
    "\r\n"
    "Compiled at: " __DATE__ " " __TIME__ "\r\n"
    "With GCC " __VERSION__ "\r\n";

#define URL "https://github.com/martinribelotta/h7-boot"

static const uint8_t index_data[] = //
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<title>Redirecting...</title>"
    "<meta http-equiv=\"refresh\" content=\"0; url=" URL "\"/>"
    "</head>"
    "<body>"
    "<p>Redirect to <a href=\"" URL "\">" URL "</a></p>"
    "</body>"
    "</html>";

void rofile_read_proc(uint8_t *dest, int size, uint32_t offset, size_t userdata)
{
    uint8_t *src = (uint8_t *)userdata;
    memcpy(dest, src + offset, size);
}

void CURRENT_read_proc(uint8_t *dest, int size, uint32_t offset, size_t userdata)
{
    UF2_Block *bl = (void *)dest;
    bl->magicStart0 = UF2_MAGIC_START0;
    bl->magicStart1 = UF2_MAGIC_START1;
    bl->magicEnd = UF2_MAGIC_END;
    bl->payloadSize = 256;
    bl->numBlocks = 4 * 1024 * 1024 / 256;
    bl->targetAddr = offset / 2;
    bl->blockNo = bl->targetAddr / 256;
    MX_QUADSPI_Read(bl->data, bl->targetAddr, 256);
}

void CURRENTbin_read_proc(uint8_t *dest, int size, uint32_t offset, size_t userdata)
{
    MX_QUADSPI_Read(dest, offset, size);
}

static inline bool is_uf2_block(UF2_Block const *bl)
{
    return (bl->magicStart0 == UF2_MAGIC_START0) && (bl->magicStart1 == UF2_MAGIC_START1) &&
           (bl->magicEnd == UF2_MAGIC_END);
}

static inline bool is_uf2_flag_set(UF2_Block const *bl, uint32_t mask)
{
    return (bl->flags & mask) == mask;
}

void CURRENT_write_proc(const uint8_t *src, int size, uint32_t offset, size_t userdata)
{
    const UF2_Block *block = (const UF2_Block *)src;
    if (is_uf2_block(block) && !is_uf2_flag_set(block, UF2_FLAG_NOFLASH)) {
        // printf("UF2 write at 0x%08lX %ld bytes. %ld of %ld\n", block->targetAddr,
        //     block->payloadSize, block->blockNo, block->numBlocks);
        if (board_flash_write(block->targetAddr, block->data, block->payloadSize) != BFLASH_RET_OK) {
            printf("Flash write fail at addr %08lX\n", block->targetAddr);
            goto fail;
        }
        if (block->blockNo == (block->numBlocks - 1)) {
            if (board_flash_flush() != BFLASH_RET_OK) {
                printf("Flash flush fail at block %ld of %ld\n", block->blockNo, block->numBlocks);
                goto fail;
            }
        }
    } else {
        // printf("Writing %d at %ld offset\n", size, offset);
        extern void usbd_storage_trigger_error(int);
    fail:
        usbd_storage_trigger_error(USBD_FAIL);
    }
}

static emfat_entry_t entries[] = {
    ROOT,                                                                     //
    ARRAY_FILE("INFO_UF2.TXT", 1, 0, info_uf2_data),                          //
    ARRAY_FILE("INDEX.HTM", 1, 0, index_data),                                //
    BIN_DYN_FILE("CURRENT.UF2", 1, 0, CURRENT_read_proc, CURRENT_write_proc), //
    BIN_DYN_FILE("CURRENT.BIN", 1, 0, CURRENTbin_read_proc, NULL),            //
    ENDROOT                                                                   //
};

void EMFATDisk_Init(void)
{
    entries[3].curr_size = entries[3].max_size = 2 * 4096 * 1024;
    entries[4].curr_size = entries[4].max_size = 4096 * 1024;
    emfat_init(&emfat, "emfat", entries);
}
