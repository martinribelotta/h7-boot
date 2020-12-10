#include "emfat.h"

#include "quadspi.h"
#include "flash_cache.h"
#include "uf2.h"

#include "usbd_def.h"

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

static const uint8_t info_uf2_data[] = "UF2 Bootloader v1.1.3 SFA\r\n"
                                       "Model: STM32H7 Minitor\r\n"
                                       "Board-ID: STM32H7 Generic\r\n"
                                       "\r\n"
                                       "Compiled at: " __DATE__ " " __TIME__ "\r\n"
                                       "With GCC " __VERSION__ "\r\n";

static const uint8_t index_data[] = "<body>"
                                    "<h1>Generic STM32 Bootloader</h1>"
                                    "<p>See more info in: <a href=\"https://github.com/martinribelotta/h7-boot\">"
                                    "https://github.com/martinribelotta/h7-boot</a></p>"
                                    "</body>";

void rofile_read_proc(uint8_t *dest, int size, uint32_t offset, size_t userdata)
{
    uint8_t *src = (uint8_t *)userdata;
    memcpy(dest, src + offset, size);
}

void CURRENT_read_proc(uint8_t *dest, int size, uint32_t offset, size_t userdata)
{
    MX_QUADSPI_Read(dest, offset, size);
}

bool is_uf2_block (UF2_Block const *bl)
{
  return (bl->magicStart0 == UF2_MAGIC_START0) &&
         (bl->magicStart1 == UF2_MAGIC_START1) &&
         (bl->magicEnd == UF2_MAGIC_END); //&&
         //!(bl->flags & UF2_FLAG_NOFLASH);
}

void CURRENT_write_proc(const uint8_t *src, int size, uint32_t offset, size_t userdata)
{
    const UF2_Block *block = (const UF2_Block *) src;
    if (is_uf2_block(block)) {
        board_flash_write(block->targetAddr, block->data, block->payloadSize);
    } else {
        extern void usbd_storage_trigger_error(int);
        usbd_storage_trigger_error(USBD_FAIL);
    }
}

static emfat_entry_t entries[] = {
    ROOT,                                                                     //
    ARRAY_FILE("INFO_UF2.TXT", 1, 0, info_uf2_data),                          //
    ARRAY_FILE("INDEX.HTM", 1, 0, index_data),                                //
    BIN_DYN_FILE("CURRENT.UF2", 1, 0, CURRENT_read_proc, CURRENT_write_proc), //
    ENDROOT                                                                   //
};

void EMFATDisk_Init(void)
{
    entries[3].curr_size = entries[3].max_size = 4096*1024;
    emfat_init(&emfat, "emfat", entries);
}
